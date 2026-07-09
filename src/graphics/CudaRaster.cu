#include "CudaRaster.cuh"
#include <cstdint>
#include <cstdlib>

namespace cuda {

// ── packed (orderable-depth << 32 | color) z-buffer helpers ──
__device__ __forceinline__ unsigned f2ord(float z) {
    unsigned u=__float_as_uint(z); unsigned m=(u>>31)?0xFFFFFFFFu:0x80000000u; return u^m;
}
__device__ __forceinline__ float ord2f(unsigned o) {
    unsigned m=(o>>31)?0x80000000u:0xFFFFFFFFu; return __uint_as_float(o^m);
}
__device__ __forceinline__ unsigned depthKey(float z, bool less) { unsigned o=f2ord(z); return less?o:(0xFFFFFFFFu-o); }
__device__ __forceinline__ float keyToDepth(unsigned k, bool less) { unsigned o=less?k:(0xFFFFFFFFu-k); return ord2f(o); }
__device__ __forceinline__ void packWrite(unsigned long long* buf, int x, int y, int W, float z, unsigned col, bool less) {
    unsigned long long v = ((unsigned long long)depthKey(z,less)<<32) | col;
    atomicMin(&buf[(size_t)y*W+x], v);
}

__global__ void kClearPacked(unsigned long long* buf, size_t n) {
    size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x;
    if (i<n) buf[i] = ((unsigned long long)0xFFFFFFFFull<<32); // far, transparent
}

__global__ void kRasterTris(unsigned long long* buf, int W, int H, const SortedTri* tris, size_t nt,
                            int mode, core::Point eye, core::Color3 ambient,
                            const core::Light* lights, int nLights, bool less) {
    size_t t=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (t>=nt) return;
    // Shared per-pixel coverage + shading; commit packs depth+color via atomicMin.
    rasterizeTriangleInto(tris[t], W, 0, H, mode, eye, ambient, lights, nLights,
        [=] (int x, int y, float z, ImU32 col) { packWrite(buf, x, y, W, z, col, less); });
}

__global__ void kSplit(const unsigned long long* buf, unsigned* color, float* depthf, size_t n, bool less) {
    size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i>=n) return;
    unsigned long long v=buf[i]; unsigned key=(unsigned)(v>>32);
    if (key==0xFFFFFFFFu) { color[i]=0u; depthf[i]= less ? 3.4e38f : -3.4e38f; }
    else { color[i]=(unsigned)(v & 0xFFFFFFFFull); depthf[i]=keyToDepth(key,less); }
}

__device__ __forceinline__ bool depthPasses(const float* depthf, int x, int y, int W, int H, float z, bool less) {
    if ((unsigned)x>=(unsigned)W || (unsigned)y>=(unsigned)H) return false;
    float d=depthf[(size_t)y*W+x]; return less ? (z<=d) : (z>=d);
}

__global__ void kRasterLines(GBView g, unsigned* color, const float* depthf, int W, int H,
                             float cw, float ch, float scale, const ObjectSlice* sl,
                             bool depthTest, bool less, int thick) {
    size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i>=g.lineCount) return;
    core::Point a=ncsToViewport(g.getPos(g.lineIdx[2*i]),   cw, ch, scale);
    core::Point b=ncsToViewport(g.getPos(g.lineIdx[2*i+1]), cw, ch, scale);
    unsigned col=sl[g.lineObj[i]].color;
    int x0=(int)lroundf(a.x), y0=(int)lroundf(a.y), x1=(int)lroundf(b.x), y1=(int)lroundf(b.y);
    int sx0=x0, sy0=y0; bool xmajor = abs(x1-sx0) >= abs(y1-sy0);
    int span = xmajor ? (x1-sx0) : (y1-sy0); float invSpan = span!=0 ? 1.0f/span : 0.0f;
    int dx=abs(x1-x0), sx=x0<x1?1:-1, dy=-abs(y1-y0), sy=y0<y1?1:-1, err=dx+dy;
    if (thick<1) thick=1;
    while (true) {
        float z=a.z;
        if (depthTest) { float u=((xmajor?(x0-sx0):(y0-sy0)))*invSpan; z=a.z+u*(b.z-a.z); }
        for (int oy=0; oy<thick; ++oy) { int yy=y0+oy; if (yy<0||yy>=H) continue;
            for (int ox=0; ox<thick; ++ox) { int xx=x0+ox; if ((unsigned)xx>=(unsigned)W) continue;
                if (!depthTest || depthPasses(depthf,xx,yy,W,H,z,less)) color[(size_t)yy*W+xx]=col; } }
        if (x0==x1 && y0==y1) break; int e2=2*err;
        if (e2>=dy) { err+=dy; x0+=sx; } if (e2<=dx) { err+=dx; y0+=sy; }
    }
}

__global__ void kRasterPoints(GBView g, unsigned* color, const float* depthf, int W, int H,
                              float cw, float ch, float scale, const ObjectSlice* sl,
                              bool depthTest, bool less, int half) {
    size_t i=(size_t)blockIdx.x*blockDim.x+threadIdx.x; if (i>=g.pointCount) return;
    core::Point p=ncsToViewport(g.getPos(g.pointIdx[i]), cw, ch, scale);
    unsigned col=sl[g.pointObj[i]].color;
    int cx=(int)lroundf(p.x), cy=(int)lroundf(p.y);
    for (int oy=-half; oy<=half; ++oy) { int yy=cy+oy; if (yy<0||yy>=H) continue;
        for (int ox=-half; ox<=half; ++ox) { int xx=cx+ox; if ((unsigned)xx>=(unsigned)W) continue;
            if (!depthTest || depthPasses(depthf,xx,yy,W,H,p.z,less)) color[(size_t)yy*W+xx]=col; } }
}

// Premultiplied box-downsample (port of Framebuffer::Resolve).
__global__ void kResolve(const unsigned* color, unsigned* resolved, int W, int rW, int rH, int f) {
    int rx=blockIdx.x*blockDim.x+threadIdx.x, ry=blockIdx.y*blockDim.y+threadIdx.y;
    if (rx>=rW || ry>=rH) return;
    if (f==1) { resolved[(size_t)ry*rW+rx]=color[(size_t)ry*W+rx]; return; }
    unsigned sumA=0,sumR=0,sumG=0,sumB=0; int bx=rx*f, by=ry*f;
    for (int dy=0; dy<f; ++dy) for (int dx=0; dx<f; ++dx) {
        unsigned p=color[(size_t)(by+dy)*W+(bx+dx)];
        unsigned a=(p>>IM_COL32_A_SHIFT)&0xFFu; sumA+=a;
        sumR+=((p>>IM_COL32_R_SHIFT)&0xFFu)*a; sumG+=((p>>IM_COL32_G_SHIFT)&0xFFu)*a; sumB+=((p>>IM_COL32_B_SHIFT)&0xFFu)*a;
    }
    float invN=1.0f/(float)(f*f);
    unsigned outA=(unsigned)(sumA*invN+0.5f), oR=0,oG=0,oB=0;
    if (sumA>0) { oR=(unsigned)(sumR/(float)sumA+0.5f); oG=(unsigned)(sumG/(float)sumA+0.5f); oB=(unsigned)(sumB/(float)sumA+0.5f); }
    resolved[(size_t)ry*rW+rx]=IM_COL32(oR,oG,oB,outA);
}

void rasterScene(DeviceFramebuffer& fb, const SortedTri* sorted, size_t sortedCount,
                 DGeo& gRender, const ObjectSlice* dslice,
                 int mode, core::Point eye, core::Color3 ambient,
                 const core::Light* lights, int nLights,
                 bool zbuffer, bool depthLess, float cw, float ch, float scale, int supersample) {
    const int T=256; const size_t npx=(size_t)fb.W*fb.H;
    kClearPacked<<<blocks(npx,T),T>>>(fb.packed.ptr, npx);
    if (sortedCount)
        kRasterTris<<<blocks(sortedCount,T),T>>>(fb.packed.ptr, fb.W, fb.H, sorted, sortedCount,
                                                 mode, eye, ambient, lights, nLights, depthLess);
    kSplit<<<blocks(npx,T),T>>>(fb.packed.ptr, fb.color.ptr, fb.depthf.ptr, npx, depthLess);

    GBView g = gRender.view();
    if (gRender.lineCount())
        kRasterLines<<<blocks(gRender.lineCount(),T),T>>>(g, fb.color.ptr, fb.depthf.ptr, fb.W, fb.H,
                                                          cw, ch, scale, dslice, zbuffer, depthLess, supersample);
    if (gRender.pointCount())
        kRasterPoints<<<blocks(gRender.pointCount(),T),T>>>(g, fb.color.ptr, fb.depthf.ptr, fb.W, fb.H,
                                                            cw, ch, scale, dslice, zbuffer, depthLess, supersample);

    dim3 tb(16,16), gb((fb.rW+15)/16, (fb.rH+15)/16);
    kResolve<<<gb,tb>>>(fb.color.ptr, fb.resolved.ptr, fb.W, fb.rW, fb.rH, fb.factor);
}

} // namespace cuda
