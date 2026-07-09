#include "RendererClipping.hpp"
#include "PipelineStages.hpp"
#include "ParallelUtils.hpp"
#include <atomic>
#include <vector>
#include <cstdint>

// CPU clip driver. The per-primitive clip math lives in PipelineStages.hpp (shared
// with the CUDA kernels); here we only provide the host "sink" (atomic-append into a
// pre-sized output GeometryBuffer) and the parallel loops. The device sink in
// CudaCommon.cuh is the same shape with atomicAdd instead of std::atomic.

namespace {
// Atomic-append sink writing into a pre-sized output buffer via the shared writeX().
struct HostClipSink {
    GBView out;
    std::atomic<uint32_t> vCount{0}, pCount{0}, lCount{0}, tCount{0};
    void emitPoint(const core::Point& p, int obj) {
        uint32_t v = vCount.fetch_add(1, std::memory_order_relaxed);
        uint32_t pi = pCount.fetch_add(1, std::memory_order_relaxed);
        writePoint(out, v, pi, p, obj);
    }
    void emitLine(const core::Point& a, const core::Point& b, int obj) {
        uint32_t v = vCount.fetch_add(2, std::memory_order_relaxed);
        uint32_t li = lCount.fetch_add(1, std::memory_order_relaxed);
        writeLine(out, v, li, a, b, obj);
    }
    void emitTri(const ClipVert& A, const ClipVert& B, const ClipVert& C, int obj) {
        uint32_t v = vCount.fetch_add(3, std::memory_order_relaxed);
        uint32_t ti = tCount.fetch_add(1, std::memory_order_relaxed);
        writeTri(out, v, ti, A, B, C, obj);
    }
};

// Worst-case pre-size, run the per-primitive clip kernels (atomic append), then
// shrink the output's logical sizes to the realized counts.
GeometryBuffer runClip(const GeometryBuffer& in, bool isNear,
                       const std::vector<ObjectSlice>* slices,
                       ClipBox box, float nearZ, int mode) {
    GeometryBuffer out;
    const bool shaded = in.shaded();
    const size_t nP = in.pointCount(), nL = in.lineCount(), nT = in.triCount();
    const size_t kTri = isNear ? 1 : 5;                 // near keeps whole; box → ≤5 fan tris
    const size_t wV = kTri * 3 * nT + 2 * nL + nP, wT = kTri * nT;

    out.pos.assign(3 * wV, 0.0f);
    if (shaded) { out.world.assign(3 * wV, 0.0f); out.normal.assign(3 * wV, 0.0f); }
    out.pointIdx.assign(nP, 0); out.pointObj.assign(nP, 0);
    out.lineIdx.assign(2 * nL, 0); out.lineObj.assign(nL, 0);
    out.triIdx.assign(3 * wT, 0); out.triObj.assign(wT, 0);

    GBView gin = const_cast<GeometryBuffer&>(in).view(); // read-only use
    HostClipSink sink; sink.out = out.view();
    const ObjectSlice* sl = slices ? slices->data() : nullptr;

    if (isNear) {
        cg_parallel_chunks(nP, [&](size_t lo, size_t hi){ for (size_t i=lo;i<hi;++i) clipPointNear(gin,i,nearZ,sink); });
        cg_parallel_chunks(nL, [&](size_t lo, size_t hi){ for (size_t i=lo;i<hi;++i) clipLineNear(gin,i,nearZ,sink); });
        cg_parallel_chunks(nT, [&](size_t lo, size_t hi){ for (size_t i=lo;i<hi;++i) clipTriNear(gin,i,nearZ,sink); });
    } else {
        cg_parallel_chunks(nP, [&](size_t lo, size_t hi){ for (size_t i=lo;i<hi;++i) clipPointBox(gin,i,box,sink); });
        cg_parallel_chunks(nL, [&](size_t lo, size_t hi){ for (size_t i=lo;i<hi;++i) clipLineBox(gin,sl,i,box,mode,sink); });
        cg_parallel_chunks(nT, [&](size_t lo, size_t hi){ for (size_t i=lo;i<hi;++i) clipTriBox(gin,sl,i,box,sink); });
    }

    const uint32_t vc = sink.vCount.load(), pc = sink.pCount.load(),
                   lc = sink.lCount.load(), tc = sink.tCount.load();
    out.pos.resize(3 * vc); if (shaded) { out.world.resize(3 * vc); out.normal.resize(3 * vc); }
    out.pointIdx.resize(pc); out.pointObj.resize(pc);
    out.lineIdx.resize(2 * lc); out.lineObj.resize(lc);
    out.triIdx.resize(3 * tc); out.triObj.resize(tc);
    return out;
}
} // namespace

GeometryBuffer NearClipFlat(const GeometryBuffer& in, float near_z) {
    return runClip(in, /*isNear*/true, nullptr, ClipBox{}, near_z, 0);
}

GeometryBuffer BoxClipFlat(const GeometryBuffer& in, const std::vector<ObjectSlice>& slices,
                           const core::Point& wp0, const core::Point& wp1, int line_clip_mode) {
    return runClip(in, /*isNear*/false, &slices, ClipBox{wp0, wp1}, 0.0f, line_clip_mode);
}

bool ClipLine(core::Line& line, const core::Point& wp0, const core::Point& wp1) {
    ClipBox box{wp0, wp1};
    return clipSegmentLB(line.a, line.b, box);
}
