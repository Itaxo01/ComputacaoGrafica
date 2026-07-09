// CUDA pipeline orchestration: holds the persistent device buffers (Context) and
// drives the per-cache-miss geometry build and the per-frame rasterize by calling the
// launchers in CudaGeometry.cu / CudaRaster.cu. All the actual math is the shared
// PipelineStages.hpp code; this file just moves data and sequences kernel launches.
#include "CudaPipeline.hpp"
#include "CudaCommon.cuh"
#include "CudaGeometry.cuh"
#include "CudaRaster.cuh"
#include <GL/gl.h>
#include <cuda_gl_interop.h>
#include <vector>

namespace cuda {

struct Context {
    DGeo gUpload;   // indexed upload; transform runs in place here
    DGeo gNear;     // near-clip output (perspective path)
    DGeo gRender;   // box-clip output: geometry rasterized this frame
    DBuf<ObjectSlice> dslice;
    DBuf<SortedTri>   dsorted;
    size_t            sortedCount = 0;
    DBuf<unsigned>    clipCounters; // [vert,point,line,tri] scratch for clip
    DBuf<unsigned>    sortCounter;  // sorted-tri append counter
    DBuf<core::Light> dlights;
    DeviceFramebuffer fb;
    GeomParams        gp;           // params from the last geometry build

    // CUDA-OpenGL interop: the registered GL texture (keyed by id+size).
    cudaGraphicsResource* glRes = nullptr;
    unsigned glTex = 0; int glW = 0, glH = 0;
    bool glInteropFailed = false;   // sticky: don't retry registration every frame
};

bool available() {
    int n=0; if (cudaGetDeviceCount(&n) != cudaSuccess) return false; return n>0;
}
Context* createContext() { if (!available()) return nullptr; return new Context(); }
void destroyContext(Context* c) {
    if (!c) return;
    if (c->glRes) cudaGraphicsUnregisterResource(c->glRes);
    c->gUpload.release(); c->gNear.release(); c->gRender.release();
    c->dslice.release(); c->dsorted.release(); c->clipCounters.release();
    c->sortCounter.release(); c->dlights.release(); c->fb.release();
    delete c;
}

// Register the GL texture with CUDA (re-registering when the id/size changes). Returns
// false if interop is unavailable — the caller then falls back to a host copy.
static bool ensureGLRegistered(Context* c, unsigned tex, int w, int h) {
    if (c->glInteropFailed || tex == 0) return false;
    if (c->glRes && c->glTex == tex && c->glW == w && c->glH == h) return true;
    if (c->glRes) { cudaGraphicsUnregisterResource(c->glRes); c->glRes = nullptr; }
    cudaError_t e = cudaGraphicsGLRegisterImage(&c->glRes, tex, GL_TEXTURE_2D,
                                                cudaGraphicsRegisterFlagsWriteDiscard);
    if (e != cudaSuccess) {
        c->glRes = nullptr; c->glInteropFailed = true; // GL context not shareable: stop trying
        cudaGetLastError();
        return false;
    }
    c->glTex = tex; c->glW = w; c->glH = h;
    return true;
}

// Copy the device resolved image straight into the registered GL texture. No host
// round-trip. Returns false on any interop error (caller falls back to D2H).
static bool blitToGLTexture(Context* c, int w, int h) {
    if (!c->glRes) return false;
    if (cudaGraphicsMapResources(1, &c->glRes, 0) != cudaSuccess) return false;
    cudaArray_t arr = nullptr;
    bool ok = (cudaGraphicsSubResourceGetMappedArray(&arr, c->glRes, 0, 0) == cudaSuccess);
    if (ok)
        ok = (cudaMemcpy2DToArray(arr, 0, 0, c->fb.resolved.ptr,
                                  (size_t)w*sizeof(unsigned), (size_t)w*sizeof(unsigned), h,
                                  cudaMemcpyDeviceToDevice) == cudaSuccess);
    cudaGraphicsUnmapResources(1, &c->glRes, 0);
    return ok;
}

template <class T>
static void uploadVec(DBuf<T>& dst, const ::std::vector<T>& src) {
    dst.ensure(src.size());
    if (!src.empty())
        CUDA_CHECK(cudaMemcpy(dst.ptr, src.data(), src.size()*sizeof(T), cudaMemcpyHostToDevice));
}

static void uploadIndexed(DGeo& g, const GeometryBuffer& in, bool shading) {
    g.shaded = shading;
    const size_t nv = in.vertexCount();
    uploadVec(g.pos, in.pos);
    uploadVec(g.vobj, in.vobj);
    if (shading) { g.world.ensure(3*nv); g.normal.ensure(3*nv); }
    uploadVec(g.pointIdx, in.pointIdx); uploadVec(g.lineIdx, in.lineIdx); uploadVec(g.triIdx, in.triIdx);
    uploadVec(g.pointObj, in.pointObj); uploadVec(g.lineObj, in.lineObj); uploadVec(g.triObj, in.triObj);
}

void processGeometry(Context* c, const GeometryBuffer& in,
                     const std::vector<ObjectSlice>& slices, const GeomParams& gp) {
    if (!c) return;
    c->gp = gp;
    const bool shading = gp.shading;
    const core::mat4 ncs = gp.perspective ? gp.vrc : gp.windowNcs;

    c->dslice.ensure(slices.size());
    if (!slices.empty())
        CUDA_CHECK(cudaMemcpy(c->dslice.ptr, slices.data(), slices.size()*sizeof(ObjectSlice), cudaMemcpyHostToDevice));

    uploadIndexed(c->gUpload, in, shading);
    launchTransform(c->gUpload, c->dslice.ptr, ncs, shading);

    DGeo* render;
    const ClipBox box{ gp.clipMin, gp.clipMax };
    if (gp.perspective) {
        launchClip(c->gUpload, c->gNear, /*isNear*/true, c->dslice.ptr, ClipBox{}, gp.nearZ, 0, c->clipCounters);
        launchProject(c->gNear, gp.projScale);
        launchClip(c->gNear, c->gRender, /*isNear*/false, c->dslice.ptr, box, 0.0f, gp.clippingMode, c->clipCounters);
    } else {
        launchClip(c->gUpload, c->gRender, /*isNear*/false, c->dslice.ptr, box, 0.0f, gp.clippingMode, c->clipCounters);
    }
    render = &c->gRender;

    const bool doSort = gp.is3d && gp.depthSort && !gp.zBuffer;
    c->sortedCount = launchBuildSorted(*render, c->dslice.ptr, c->dsorted, c->sortCounter,
                                       gp.canvasW, gp.canvasH, gp.scale,
                                       gp.is3d && gp.backfaceCull, gp.cullCcw, doSort, gp.depthAscending);
    CUDA_CHECK(cudaDeviceSynchronize());
}

void rasterize(Context* c, const FrameParams& fp, unsigned glTexture,
               ImU32* outResolved, bool* outUsedInterop) {
    if (outUsedInterop) *outUsedInterop = false;
    if (!c) return;
    const int ss = fp.supersample < 1 ? 1 : fp.supersample;
    const int rW=fp.displayW, rH=fp.displayH, W=rW*ss, H=rH*ss;
    if (W<=0 || H<=0) return;
    c->fb.ensure(W,H,rW,rH,ss);

    c->dlights.ensure(fp.lights.size());
    if (!fp.lights.empty())
        CUDA_CHECK(cudaMemcpy(c->dlights.ptr, fp.lights.data(), fp.lights.size()*sizeof(core::Light), cudaMemcpyHostToDevice));

    rasterScene(c->fb, c->dsorted.ptr, c->sortedCount, c->gRender, c->dslice.ptr,
                fp.shadeMode, fp.eye, fp.ambient, c->dlights.ptr, (int)fp.lights.size(),
                fp.zBuffer, fp.depthLess, c->gp.canvasW, c->gp.canvasH, c->gp.scale, ss);

    // Preferred: blit the resolved image straight into the GL texture (no host copy).
    if (ensureGLRegistered(c, glTexture, rW, rH) && blitToGLTexture(c, rW, rH)) {
        if (outUsedInterop) *outUsedInterop = true;
        CUDA_CHECK(cudaDeviceSynchronize());
        return;
    }
    // Fallback: device -> host copy for Framebuffer::PresentExternal.
    if (outResolved)
        CUDA_CHECK(cudaMemcpy(outResolved, c->fb.resolved.ptr, (size_t)rW*rH*sizeof(unsigned), cudaMemcpyDeviceToHost));
}

} // namespace cuda
