#pragma once

// CG_HD marks functions that must run on BOTH the CPU and the GPU. Under nvcc it
// expands to `__host__ __device__`; under a normal g++/clang build it expands to
// nothing, so the CPU and mingw-Windows builds are completely unaffected. This lets
// the per-element pipeline math live in one place (shared headers) and be called by
// both the CPU drivers (cg_parallel_chunks loops) and the CUDA kernels.
#if defined(__CUDACC__)
    #define CG_HD __host__ __device__
#else
    #define CG_HD
#endif
