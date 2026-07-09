#pragma once
// Shared CUDA device infrastructure: error checking, grow-only device buffers, the
// device geometry/framebuffer holders, and the device sinks (atomic-append using the
// SAME writeX() / SortedTri helpers as the CPU path in PipelineStages.hpp).
#include <cuda_runtime.h>
#include <cstdio>
#include <cstdint>
#include "GeometryView.hpp"
#include "ObjectSlice.hpp"
#include "RenderedObject.hpp" // SortedTri
#include "PipelineStages.hpp" // writePoint/writeLine/writeTri, ClipVert

namespace cuda {

// NOTE: ::std::fprintf (not std::fprintf) — inside `namespace cuda`, an unqualified
// `std` resolves to libcu++'s `cuda::std` (pulled in by thrust), which has no fprintf.
#define CUDA_CHECK(expr) do {                                                       \
    cudaError_t _e = (expr);                                                        \
    if (_e != cudaSuccess)                                                          \
        ::std::fprintf(stderr, "CUDA error %s @ %s:%d: %s\n", #expr, __FILE__,     \
                       __LINE__, cudaGetErrorString(_e));                          \
} while (0)

static inline int blocks(size_t n, int t) { return (int)((n + t - 1) / t); }

// Grow-only device allocation. `size` is the logical length; `cap` the allocation.
template <class T> struct DBuf {
    T* ptr = nullptr; size_t cap = 0, size = 0;
    void ensure(size_t n) {
        if (n > cap) { if (ptr) cudaFree(ptr); CUDA_CHECK(cudaMalloc(&ptr, (n ? n : 1) * sizeof(T))); cap = n; }
        size = n;
    }
    void release() { if (ptr) cudaFree(ptr); ptr = nullptr; cap = size = 0; }
};

// Device mirror of GeometryBuffer (SoA). view() yields a GBView of device pointers.
struct DGeo {
    DBuf<float> pos, world, normal; DBuf<int32_t> vobj;
    DBuf<uint32_t> pointIdx, lineIdx, triIdx; DBuf<int32_t> pointObj, lineObj, triObj;
    bool shaded = false;
    size_t vertexCount() const { return pos.size/3; }
    size_t pointCount()  const { return pointIdx.size; }
    size_t lineCount()   const { return lineIdx.size/2; }
    size_t triCount()    const { return triIdx.size/3; }
    GBView view() {
        GBView v;
        v.pos=pos.ptr; v.world=shaded?world.ptr:nullptr; v.normal=shaded?normal.ptr:nullptr;
        v.vobj=vobj.ptr;
        v.pointIdx=pointIdx.ptr; v.lineIdx=lineIdx.ptr; v.triIdx=triIdx.ptr;
        v.pointObj=pointObj.ptr; v.lineObj=lineObj.ptr; v.triObj=triObj.ptr;
        v.vertexCount=vertexCount(); v.pointCount=pointCount();
        v.lineCount=lineCount(); v.triCount=triCount(); v.shaded=shaded;
        return v;
    }
    void release() {
        pos.release(); world.release(); normal.release(); vobj.release();
        pointIdx.release(); lineIdx.release(); triIdx.release();
        pointObj.release(); lineObj.release(); triObj.release();
    }
};

// Atomic-append clip sink (mirror of the host HostClipSink). Passed BY VALUE to
// kernels: the GBView/pointers are shared and the atomics operate through `ctr`.
// ctr = [vertCount, pointCount, lineCount, triCount].
struct DeviceClipSink {
    GBView out; unsigned* ctr;
    __device__ void emitPoint(const core::Point& p, int obj) {
        unsigned v=atomicAdd(&ctr[0],1u), pi=atomicAdd(&ctr[1],1u); writePoint(out,v,pi,p,obj);
    }
    __device__ void emitLine(const core::Point& a, const core::Point& b, int obj) {
        unsigned v=atomicAdd(&ctr[0],2u), li=atomicAdd(&ctr[2],1u); writeLine(out,v,li,a,b,obj);
    }
    __device__ void emitTri(const ClipVert& A, const ClipVert& B, const ClipVert& C, int obj) {
        unsigned v=atomicAdd(&ctr[0],3u), ti=atomicAdd(&ctr[3],1u); writeTri(out,v,ti,A,B,C,obj);
    }
};
struct DeviceSortedSink {
    SortedTri* out; unsigned* ctr;
    __device__ void emit(const SortedTri& t) { unsigned i=atomicAdd(ctr,1u); out[i]=t; }
};

// Device render target: packed (depth<<32|color) z-buffer, split color/depth, and the
// downsampled display-resolution image. All at supersample resolution except resolved.
struct DeviceFramebuffer {
    DBuf<unsigned long long> packed;
    DBuf<unsigned>           color;
    DBuf<float>              depthf;
    DBuf<unsigned>           resolved;
    int W=0,H=0,rW=0,rH=0,factor=1;
    void ensure(int w,int h,int rw,int rh,int f) {
        W=w;H=h;rW=rw;rH=rh;factor=f;
        packed.ensure((size_t)w*h); color.ensure((size_t)w*h);
        depthf.ensure((size_t)w*h); resolved.ensure((size_t)rw*rh);
    }
    void release() { packed.release(); color.release(); depthf.release(); resolved.release(); }
};

} // namespace cuda
