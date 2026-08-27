#pragma once
#include <cstddef>
#include <cstdint>

// Allocation counters for the headless benchmark.
//
// BenchAlloc.cpp replaces the global operator new/delete for the BENCH BINARY
// ONLY (src/bench is not linked into programa_foda.out). The point is to answer
// "how many allocations does one frame actually cost?" directly, in numbers,
// instead of inferring it from the ~1% of cycles glibc's malloc shows in a perf
// profile — a cheap allocation that happens 300k times per frame is a real cost
// even when no single call is expensive.
//
// Counting is OFF by default and gated on a plain (non-atomic) bool, so timing
// runs are not skewed by the counters: when disabled, the replacement operator
// new is a predictable-branch + malloc, which is what the default one is anyway.
//
// Caveat to keep in mind when reading the numbers: this counts C++ operator new,
// not raw malloc/realloc. Anything allocating through the C API directly (inside
// libc, GLFW, TBB's own scalable allocator) is invisible here. For the C++
// containers this project uses — which is the whole question — operator new is
// the right hook.
namespace bench {

struct AllocSnapshot {
    uint64_t allocs = 0;   // operator new calls
    uint64_t frees  = 0;   // operator delete calls
    uint64_t bytes  = 0;   // total bytes requested through operator new
};

// Enable/disable counting. Call with the measured region as narrow as possible.
void AllocCountingEnable(bool on);
bool AllocCountingEnabled();

// Zero the counters (does not change the enabled flag).
void AllocReset();

// Read the counters. Safe to call at any time.
AllocSnapshot AllocRead();

} // namespace bench
