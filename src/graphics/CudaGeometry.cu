#include "CudaGeometry.cuh"
#include <thrust/sort.h>
#include <thrust/execution_policy.h>

// Geometry kernels: each is a thin wrapper around the shared per-element bodies in
// PipelineStages.hpp (the very same code the CPU drivers run). The only GPU-specific
// pieces are the grid indexing, the atomic-append sinks, and the thrust sort.
namespace cuda {

// ── per-vertex / per-triangle kernels ──
__global__ void kTransform(GBView g, const ObjectSlice* sl, core::mat4 ncs, bool shaded, size_t nv) {
    size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<nv) transformVertex(g, sl, ncs, shaded, i);
}
__global__ void kNormalZero(float* n, size_t n3) {
    size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<n3) n[i]=0.0f;
}
__global__ void kNormalScatter(GBView g, size_t nt) {
    size_t t=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (t>=nt) return;
    core::Point fn = faceNormalWorld(g, t);
    uint32_t a=g.triIdx[3*t], b=g.triIdx[3*t+1], c=g.triIdx[3*t+2];
    atomicAdd(&g.normal[3*a],fn.x); atomicAdd(&g.normal[3*a+1],fn.y); atomicAdd(&g.normal[3*a+2],fn.z);
    atomicAdd(&g.normal[3*b],fn.x); atomicAdd(&g.normal[3*b+1],fn.y); atomicAdd(&g.normal[3*b+2],fn.z);
    atomicAdd(&g.normal[3*c],fn.x); atomicAdd(&g.normal[3*c+1],fn.y); atomicAdd(&g.normal[3*c+2],fn.z);
}
__global__ void kNormalize(GBView g, size_t nv) {
    size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<nv) normalizeVertexNormal(g, i);
}
__global__ void kProject(GBView g, core::mat4 m, size_t nv) {
    size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<nv) projectVertex(g, m, i);
}

// ── clip kernels (sink passed by value) ──
__global__ void kClipPointNear(GBView in, DeviceClipSink s, float nz)   { size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<in.pointCount) clipPointNear(in,i,nz,s); }
__global__ void kClipLineNear (GBView in, DeviceClipSink s, float nz)   { size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<in.lineCount)  clipLineNear (in,i,nz,s); }
__global__ void kClipTriNear  (GBView in, DeviceClipSink s, float nz)   { size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<in.triCount)   clipTriNear  (in,i,nz,s); }
__global__ void kClipPointBox (GBView in, DeviceClipSink s, ClipBox b)  { size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<in.pointCount) clipPointBox (in,i,b,s); }
__global__ void kClipLineBox  (GBView in, const ObjectSlice* sl, DeviceClipSink s, ClipBox b, int mode) { size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<in.lineCount) clipLineBox(in,sl,i,b,mode,s); }
__global__ void kClipTriBox   (GBView in, const ObjectSlice* sl, DeviceClipSink s, ClipBox b)           { size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i<in.triCount)  clipTriBox (in,sl,i,b,s); }

// Painter's-order comparator. Must live at namespace scope: thrust/cub instantiates
// device kernels templated on this type, and a function-local type can't be a kernel
// template argument.
struct SortedDepthCmp {
    bool ascending;
    __host__ __device__ bool operator()(const SortedTri& a, const SortedTri& b) const {
        return ascending ? (a.depth < b.depth) : (a.depth > b.depth);
    }
};

// ── build sorted triangles ──
__global__ void kBuildSorted(GBView g, const ObjectSlice* sl, DeviceSortedSink s,
                             float cw, float ch, float scale, int meshType, bool cull, bool ccw) {
    size_t t=(size_t)blockIdx.x*blockDim.x+threadIdx.x;
    if (t<g.triCount) buildSortedTriangle(g, sl, t, cw, ch, scale, meshType, cull, ccw, s);
}

// ── launchers ──
void launchTransform(DGeo& g, const ObjectSlice* dslice, const core::mat4& ncs, bool shaded) {
    const size_t nv=g.vertexCount(); if(!nv) return; const int T=256;
    kTransform<<<blocks(nv,T),T>>>(g.view(), dslice, ncs, shaded, nv);
    if (shaded) {
        kNormalZero<<<blocks(3*nv,T),T>>>(g.normal.ptr, 3*nv);
        if (g.triCount()) kNormalScatter<<<blocks(g.triCount(),T),T>>>(g.view(), g.triCount());
        kNormalize<<<blocks(nv,T),T>>>(g.view(), nv);
    }
}
void launchProject(DGeo& g, const core::mat4& mat) {
    const size_t nv=g.vertexCount(); if(!nv) return; const int T=256;
    kProject<<<blocks(nv,T),T>>>(g.view(), mat, nv);
}

void launchClip(const DGeo& in, DGeo& out, bool isNear, const ObjectSlice* dslice,
                const ClipBox& box, float nearZ, int mode, DBuf<unsigned>& counters) {
    DGeo& cin = const_cast<DGeo&>(in);
    const bool shaded = in.shaded; out.shaded = shaded;
    const size_t nP=in.pointCount(), nL=in.lineCount(), nT=in.triCount();
    const size_t kTri = isNear ? 1 : 5;
    const size_t wV = kTri*3*nT + 2*nL + nP, wT = kTri*nT;

    out.pos.ensure(3*wV); if (shaded) { out.world.ensure(3*wV); out.normal.ensure(3*wV); }
    out.pointIdx.ensure(nP); out.pointObj.ensure(nP);
    out.lineIdx.ensure(2*nL); out.lineObj.ensure(nL);
    out.triIdx.ensure(3*wT); out.triObj.ensure(wT);

    counters.ensure(4);
    CUDA_CHECK(cudaMemset(counters.ptr, 0, 4*sizeof(unsigned)));
    GBView gin = cin.view();
    DeviceClipSink sink{ out.view(), counters.ptr };
    const int T=256;
    if (isNear) {
        if (nP) kClipPointNear<<<blocks(nP,T),T>>>(gin, sink, nearZ);
        if (nL) kClipLineNear <<<blocks(nL,T),T>>>(gin, sink, nearZ);
        if (nT) kClipTriNear  <<<blocks(nT,T),T>>>(gin, sink, nearZ);
    } else {
        if (nP) kClipPointBox<<<blocks(nP,T),T>>>(gin, sink, box);
        if (nL) kClipLineBox <<<blocks(nL,T),T>>>(gin, dslice, sink, box, mode);
        if (nT) kClipTriBox  <<<blocks(nT,T),T>>>(gin, dslice, sink, box);
    }
    unsigned h[4]={0,0,0,0};
    CUDA_CHECK(cudaMemcpy(h, counters.ptr, 4*sizeof(unsigned), cudaMemcpyDeviceToHost));
    out.pos.size=3*h[0]; if (shaded) { out.world.size=3*h[0]; out.normal.size=3*h[0]; }
    out.pointIdx.size=h[1]; out.pointObj.size=h[1];
    out.lineIdx.size=2*h[2]; out.lineObj.size=h[2];
    out.triIdx.size=3*h[3]; out.triObj.size=h[3];
}

size_t launchBuildSorted(DGeo& g, const ObjectSlice* dslice, DBuf<SortedTri>& outSorted,
                         DBuf<unsigned>& counter, float cw, float ch, float scale,
                         bool cull, bool cullCcw, bool doSort, bool ascending) {
    const size_t nt=g.triCount();
    outSorted.ensure(nt ? nt : 1);
    counter.ensure(1);
    CUDA_CHECK(cudaMemset(counter.ptr, 0, sizeof(unsigned)));
    if (nt) {
        const int T=256;
        DeviceSortedSink s{ outSorted.ptr, counter.ptr };
        kBuildSorted<<<blocks(nt,T),T>>>(g.view(), dslice, s, cw, ch, scale,
                                         (int)core::ObjectType::MESH, cull, cullCcw);
    }
    unsigned cnt=0; CUDA_CHECK(cudaMemcpy(&cnt, counter.ptr, sizeof(unsigned), cudaMemcpyDeviceToHost));
    if (doSort && cnt>1)
        thrust::sort(thrust::device, outSorted.ptr, outSorted.ptr+cnt, SortedDepthCmp{ascending});
    return cnt;
}

} // namespace cuda
