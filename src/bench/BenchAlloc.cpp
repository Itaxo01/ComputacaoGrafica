#include "BenchAlloc.hpp"
#include <atomic>
#include <cstdlib>
#include <new>

namespace {
    // Non-atomic on purpose: flipped only from the main thread between measured
    // regions, never mid-region, so a relaxed data race is not possible in the
    // way it is used. Making it atomic would put a synchronized load on every
    // allocation in every worker thread, which is exactly the cost we are trying
    // not to introduce into the timing runs.
    bool g_counting = false;

    // Relaxed: we only ever want the final totals, never a happens-before
    // relationship with anything else. On x86 this compiles to a lock xadd,
    // which is why counting is opt-in — 16 threads hammering three shared cache
    // lines would otherwise dominate the very rebuild loop being measured.
    std::atomic<uint64_t> g_allocs{0};
    std::atomic<uint64_t> g_frees{0};
    std::atomic<uint64_t> g_bytes{0};

    inline void* do_alloc(std::size_t n) {
        if (g_counting) {
            g_allocs.fetch_add(1, std::memory_order_relaxed);
            g_bytes.fetch_add(n, std::memory_order_relaxed);
        }
        // operator new must never return nullptr for a zero-sized request.
        void* p = std::malloc(n ? n : 1);
        if (!p) throw std::bad_alloc();
        return p;
    }

    inline void* do_alloc_aligned(std::size_t n, std::size_t align) {
        if (g_counting) {
            g_allocs.fetch_add(1, std::memory_order_relaxed);
            g_bytes.fetch_add(n, std::memory_order_relaxed);
        }
        // aligned_alloc requires the size to be a multiple of the alignment.
        std::size_t sz = ((n ? n : 1) + align - 1) / align * align;
        void* p = std::aligned_alloc(align, sz);
        if (!p) throw std::bad_alloc();
        return p;
    }

    inline void do_free(void* p) {
        if (!p) return;
        if (g_counting) g_frees.fetch_add(1, std::memory_order_relaxed);
        std::free(p);
    }
}

namespace bench {
    void AllocCountingEnable(bool on) { g_counting = on; }
    bool AllocCountingEnabled() { return g_counting; }

    void AllocReset() {
        g_allocs.store(0, std::memory_order_relaxed);
        g_frees.store(0, std::memory_order_relaxed);
        g_bytes.store(0, std::memory_order_relaxed);
    }

    AllocSnapshot AllocRead() {
        AllocSnapshot s;
        s.allocs = g_allocs.load(std::memory_order_relaxed);
        s.frees  = g_frees.load(std::memory_order_relaxed);
        s.bytes  = g_bytes.load(std::memory_order_relaxed);
        return s;
    }
}

// ─── Global replacements ─────────────────────────────────────────────────────
// The whole set has to be replaced together: mixing our operator new with the
// library's operator delete (or vice versa) would free a malloc'd pointer with a
// different allocator. The sized and aligned overloads exist since C++14/17 and
// the compiler emits calls to them, so they are not optional.

void* operator new(std::size_t n) { return do_alloc(n); }
void* operator new[](std::size_t n) { return do_alloc(n); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
    try { return do_alloc(n); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
    try { return do_alloc(n); } catch (...) { return nullptr; }
}
void* operator new(std::size_t n, std::align_val_t a) {
    return do_alloc_aligned(n, (std::size_t)a);
}
void* operator new[](std::size_t n, std::align_val_t a) {
    return do_alloc_aligned(n, (std::size_t)a);
}
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
    try { return do_alloc_aligned(n, (std::size_t)a); } catch (...) { return nullptr; }
}
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
    try { return do_alloc_aligned(n, (std::size_t)a); } catch (...) { return nullptr; }
}

void operator delete(void* p) noexcept { do_free(p); }
void operator delete[](void* p) noexcept { do_free(p); }
void operator delete(void* p, std::size_t) noexcept { do_free(p); }
void operator delete[](void* p, std::size_t) noexcept { do_free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { do_free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { do_free(p); }
void operator delete(void* p, std::align_val_t) noexcept { do_free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { do_free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { do_free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { do_free(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { do_free(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { do_free(p); }
