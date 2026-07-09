// No-CUDA fallback for the cuda:: API. This TU is always part of the build (the
// Makefile globs *.cpp), but its body only compiles when the project is built
// WITHOUT -DUSE_CUDA. In that case `available()` returns false and every other
// entry point is an inert no-op, so the Renderer always takes the CPU path.
//
// When built WITH -DUSE_CUDA, this file is empty and CudaPipeline.cu (compiled by
// nvcc in the `cuda` target) provides the real symbols.
#ifndef USE_CUDA
#include "CudaPipeline.hpp"

namespace cuda {
    bool     available() { return false; }
    Context* createContext() { return nullptr; }
    void     destroyContext(Context*) {}
    void     processGeometry(Context*, const GeometryBuffer&,
                             const std::vector<ObjectSlice>&, const GeomParams&) {}
    void     rasterize(Context*, const FrameParams&, unsigned, ImU32*, bool* used) { if (used) *used = false; }
}
#endif // USE_CUDA
