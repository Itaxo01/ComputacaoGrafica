#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Timing + summary helpers for the headless benchmark. Header-only and used only
// by src/bench — nothing in the application links against this.
namespace bench {

// Monotonic stopwatch. steady_clock and not high_resolution_clock: the latter is
// an alias for system_clock in some libstdc++ configurations, so an NTP step
// mid-run could produce a negative frame time.
class Timer {
    std::chrono::steady_clock::time_point t0;
public:
    Timer() : t0(std::chrono::steady_clock::now()) {}
    void reset() { t0 = std::chrono::steady_clock::now(); }
    double ms() const {
        return std::chrono::duration<double, std::milli>(
                   std::chrono::steady_clock::now() - t0).count();
    }
};

struct Summary {
    double mean = 0, median = 0, p95 = 0, min = 0, max = 0, total = 0;
};

// One measured pipeline stage. The measured path only ever does a push_back into
// an already-reserved vector; every statistic is derived afterwards, so the
// reporting code cannot show up in the profile of the thing being reported on.
struct Stage {
    std::string name;
    std::vector<double> samples;
    // Allocation traffic attributed to this stage, summed over measured frames.
    // Only filled when --count-allocs is on; the deltas are read on the main
    // thread after each stage's parallel region has joined, so work done on TBB
    // workers lands on the stage that spawned it.
    uint64_t allocs = 0;
    uint64_t bytes  = 0;

    explicit Stage(std::string n) : name(std::move(n)) {}
    void reserve(std::size_t n) { samples.reserve(n); }
    void add(double ms) { samples.push_back(ms); }

    Summary summarize() const {
        Summary s;
        if (samples.empty()) return s;
        std::vector<double> v = samples;
        std::sort(v.begin(), v.end());
        for (double x : v) s.total += x;
        s.mean   = s.total / (double)v.size();
        s.median = v[v.size() / 2];
        // Nearest-rank p95: the smallest sample at or above the 95th percentile.
        // With few frames this is simply the worst-but-one, which is the honest
        // answer for a short run — no interpolation pretending to more precision.
        s.p95    = v[(std::size_t)((double)(v.size() - 1) * 0.95)];
        s.min    = v.front();
        s.max    = v.back();
        return s;
    }
};

inline void PrintTable(const std::vector<Stage>& stages, int frames,
                       const char* unit = "frame") {
    double grand = 0;
    for (const auto& st : stages) grand += st.summarize().mean;

    std::printf("\n  %-22s %9s %9s %9s %9s %9s %8s\n",
                "stage", "mean ms", "median", "p95", "min", "max", "share");
    std::printf("  %s\n", std::string(22 + 9 * 5 + 8 + 6, '-').c_str());
    for (const auto& st : stages) {
        Summary s = st.summarize();
        if (s.mean == 0 && s.max == 0) continue;
        std::printf("  %-22s %9.3f %9.3f %9.3f %9.3f %9.3f %7.1f%%\n",
                    st.name.c_str(), s.mean, s.median, s.p95, s.min, s.max,
                    grand > 0 ? 100.0 * s.mean / grand : 0.0);
    }
    std::printf("  %s\n", std::string(22 + 9 * 5 + 8 + 6, '-').c_str());
    char total_label[48];
    std::snprintf(total_label, sizeof(total_label), "TOTAL / %s", unit);
    std::printf("  %-22s %9.3f %9s %9s %9s %9s %7s\n", total_label, grand, "", "", "", "", "");
    std::printf("  %-22s %9.1f   (over %d measured %ss)\n\n",
                "equivalent per second", grand > 0 ? 1000.0 / grand : 0.0, frames, unit);
}

// Per-stage allocation traffic. Printed instead of guessed at: "the pipeline
// allocates a lot" is not actionable, "stage X does N allocations per frame" is.
inline void PrintAllocTable(const std::vector<Stage>& stages, int frames) {
    uint64_t total_a = 0, total_b = 0;
    for (const auto& st : stages) { total_a += st.allocs; total_b += st.bytes; }
    if (total_a == 0) return;

    std::printf("  %-22s %12s %12s %12s\n", "stage", "new/frame", "MB/frame", "share");
    std::printf("  %s\n", std::string(22 + 12 * 3 + 4, '-').c_str());
    for (const auto& st : stages) {
        if (st.allocs == 0) continue;
        std::printf("  %-22s %12.0f %12.2f %11.1f%%\n", st.name.c_str(),
                    (double)st.allocs / frames,
                    (double)st.bytes / frames / (1024.0 * 1024.0),
                    100.0 * (double)st.allocs / (double)total_a);
    }
    std::printf("  %s\n", std::string(22 + 12 * 3 + 4, '-').c_str());
    std::printf("  %-22s %12.0f %12.2f\n\n", "TOTAL / frame",
                (double)total_a / frames, (double)total_b / frames / (1024.0 * 1024.0));
}

// Frame-level summary. Deliberately separate from PrintTable: the per-stage max
// column is the max of each stage taken independently, which is NOT the cost of
// the worst frame — stages peak at different moments. A scene whose cost swings
// with the camera angle (a flat model seen face-on vs edge-on) is only honestly
// described by the distribution of whole-frame totals, plus which frame was worst.
inline void PrintFrameSummary(const Stage& frame, const std::vector<float>& angles) {
    if (frame.samples.empty()) return;
    Summary s = frame.summarize();

    std::size_t worst = 0, best = 0;
    for (std::size_t i = 1; i < frame.samples.size(); ++i) {
        if (frame.samples[i] > frame.samples[worst]) worst = i;
        if (frame.samples[i] < frame.samples[best])  best  = i;
    }

    std::printf("  whole-frame totals (the number the FPS overlay would show)\n");
    std::printf("  %s\n", std::string(66, '-').c_str());
    std::printf("  %-14s %9.2f ms  %6.1f FPS\n", "mean",   s.mean,   s.mean   > 0 ? 1000.0 / s.mean   : 0.0);
    std::printf("  %-14s %9.2f ms  %6.1f FPS\n", "median", s.median, s.median > 0 ? 1000.0 / s.median : 0.0);
    std::printf("  %-14s %9.2f ms  %6.1f FPS\n", "p95",    s.p95,    s.p95    > 0 ? 1000.0 / s.p95    : 0.0);
    if (!angles.empty()) {
        std::printf("  %-14s %9.2f ms  %6.1f FPS   (frame %zu, %.1f deg)\n", "best frame",
                    s.min, s.min > 0 ? 1000.0 / s.min : 0.0, best, angles[best]);
        std::printf("  %-14s %9.2f ms  %6.1f FPS   (frame %zu, %.1f deg)\n", "WORST frame",
                    s.max, s.max > 0 ? 1000.0 / s.max : 0.0, worst, angles[worst]);
    } else {
        std::printf("  %-14s %9.2f ms  %6.1f FPS   (frame %zu)\n", "best frame",
                    s.min, s.min > 0 ? 1000.0 / s.min : 0.0, best);
        std::printf("  %-14s %9.2f ms  %6.1f FPS   (frame %zu)\n", "WORST frame",
                    s.max, s.max > 0 ? 1000.0 / s.max : 0.0, worst);
    }
    std::printf("  %-14s %9.2fx\n\n", "worst/best", s.min > 0 ? s.max / s.min : 0.0);
}

// Per-frame dump, for plotting cost against rotation angle. One row per measured
// frame: the aggregate statistics above say a swing exists, this says where.
inline bool DumpFrames(const std::string& path, const std::vector<Stage>& stages,
                       const Stage& frame, const std::vector<float>& angles) {
    std::FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return false;
    std::fprintf(f, "frame,angle_deg,total_ms");
    for (const auto& st : stages)
        if (!st.samples.empty()) std::fprintf(f, ",%s", st.name.c_str());
    std::fprintf(f, "\n");
    for (std::size_t i = 0; i < frame.samples.size(); ++i) {
        std::fprintf(f, "%zu,%.3f,%.6f", i, angles.empty() ? 0.0f : angles[i], frame.samples[i]);
        for (const auto& st : stages)
            if (!st.samples.empty())
                std::fprintf(f, ",%.6f", i < st.samples.size() ? st.samples[i] : 0.0);
        std::fprintf(f, "\n");
    }
    std::fclose(f);
    return true;
}

// Machine-readable form for A/B runs: one row per stage, so two runs can be
// diffed with a spreadsheet or `join` instead of by eye.
inline void PrintCSV(const std::vector<Stage>& stages, const std::string& tag) {
    std::printf("run,stage,mean_ms,median_ms,p95_ms,min_ms,max_ms,samples,allocs,bytes\n");
    for (const auto& st : stages) {
        Summary s = st.summarize();
        std::printf("%s,%s,%.6f,%.6f,%.6f,%.6f,%.6f,%zu,%llu,%llu\n",
                    tag.c_str(), st.name.c_str(), s.mean, s.median, s.p95,
                    s.min, s.max, st.samples.size(),
                    (unsigned long long)st.allocs, (unsigned long long)st.bytes);
    }
}

} // namespace bench
