// Headless benchmark harness for the render pipeline.
//
// WHY THIS EXISTS
// The application is a GLFW/ImGui program driven by hand, which makes it a poor
// measuring instrument: frame times depend on where the mouse is, vsync clamps
// anything under 16 ms, and no two runs stress the same geometry. This binary
// runs the SAME pipeline stages the Renderer runs, over a fixed scene, for a
// fixed number of frames, with no window, no GL context and no input — so a
// before/after comparison of a refactor is a comparison of the refactor.
//
// WHAT IT IS NOT
// It is not a second renderer. Every stage below is a call into the same
// function the application calls (RendererTransform / RendererClipping /
// RasterizationEngine / Framebuffer). The only duplicated logic is the banded
// rasterization loop and the per-object line dispatch, which live as private
// methods on Renderer (Renderer owns draw dispatch by design) and are mirrored
// here in RasterizeMirror/DrawObjectMirror.
//
// KEEPING IT HONEST
// If Renderer::render() or Renderer::RasterizeFramebuffer() changes shape, this
// file must follow, or the benchmark silently starts measuring a pipeline the
// application no longer runs. The mirrored functions are marked with MIRRORS:
// comments naming their counterpart.

#include "AppConfig.hpp"
#include "DisplayFile.hpp"
#include "EntityManager.hpp"
#include "Framebuffer.hpp"
#include "Lighting.hpp"
#include "ObjSerializer.hpp"
#include "MtlSerializer.hpp"
#include "ParallelUtils.hpp"
#include "RasterizationEngine.hpp"
#include "RenderedObject.hpp"
#include "Renderer.hpp"
#include "RendererClipping.hpp"
#include "RendererTransform.hpp"
#include "Shading.hpp"
#include "Transformations.hpp"
#include "Viewport.hpp"
#include "Window.hpp"
#include "log_app.h"

#include "BenchAlloc.hpp"
#include "BenchStats.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <vector>

namespace {

// ─── Options ─────────────────────────────────────────────────────────────────

struct Options {
    std::string model  = "models/Donut.obj";
    std::string mode   = "rebuild";   // rebuild | raster | import
    std::string tag    = "run";
    int   frames  = 120;
    int   warmup  = 10;
    int   width   = 1280;             // viewport size in DISPLAY pixels
    int   height  = 720;
    int   repeat  = 3;                // import mode only
    float spin    = 1.5f;             // degrees per frame, rebuild mode
    float zoom    = 1.0f;             // >1 moves the camera in (smaller view volume)
    float start_angle = 0.0f;         // initial pose; with --spin 0 this holds one orientation
    std::string axis = "xz";          // spin axis: x | y | z | xz (xz mirrors donut_spin.txt)
    bool  sweep   = false;            // spin exactly 360 deg over the measured frames
    std::string dump_frames;          // path for the per-frame CSV, empty = off
    bool  count_allocs = false;
    bool  raster_stats = false;
    int   chunks_per_thread = 0;      // 0 = the renderer's own default (K=8)
    bool  csv = false;
};

void Usage() {
    std::printf(
"Usage: ./bench.out [options]\n"
"\n"
"  --model PATH        .obj to load                    (default models/Donut.obj)\n"
"  --mode MODE         rebuild | raster | import       (default rebuild)\n"
"                        rebuild: full pipeline every frame (mirrors an animating\n"
"                                 scene, where the geometry cache never hits)\n"
"                        raster:  geometry built once, then only rasterize+resolve\n"
"                                 (mirrors a static camera, cache hitting)\n"
"                        import:  time obj::Import only\n"
"  --frames N          measured frames                 (default 120)\n"
"  --warmup N          unmeasured frames first         (default 10)\n"
"  --width N           viewport width in px            (default 1280)\n"
"  --height N          viewport height in px           (default 720)\n"
"  --ssaa N            supersample factor 1..4         (default %d)\n"
"  --spin DEG          per-frame rotation, rebuild     (default 1.5)\n"
"  --sweep             set --spin so the measured frames cover exactly 360 deg\n"
"  --axis A            spin axis: x | y | z | xz        (default xz, as donut_spin.txt)\n"
"  --zoom F            camera zoom over the auto-fit    (default 1.0; 2.0 = twice as close)\n"
"  --start-angle DEG   initial pose; with --spin 0 it holds one orientation, which is\n"
"                      how you profile the worst angle a --sweep found\n"
"  --repeat N          imports to time, import mode    (default 3)\n"
"  --dump-frames PATH  write a per-frame CSV (angle vs cost) for plotting\n"
"\n"
"  --shading MODE      none | flat | gouraud | phong   (default none)\n"
"  --3d on|off         --perspective on|off  --zbuffer on|off\n"
"  --cull on|off       --depth-sort on|off   --clip lb|cs\n"
"\n"
"  --chunks-per-thread K   rasterizer bands per hardware thread (0 = renderer default;\n"
"                      K=1 is the old one-band-per-thread behaviour, for A/B)\n"
"  --par BACKEND       parallel scheduler: tbb | native (default: tbb when built\n"
"                      with it). Same binary either way -- this switches which\n"
"                      scheduler runs the loops, it does NOT unlink libtbb.\n"
"  --raster-stats      one untimed pass reporting draw calls, inner-loop trip count,\n"
"                      covered fragments and overdraw for the final frame\n"
"  --count-allocs      count operator new/delete (adds contention: a counting\n"
"                      run's TIMINGS are not comparable to a normal run's)\n"
"  --csv               emit CSV instead of a table\n"
"  --tag NAME          label for the CSV rows          (default \"run\")\n"
"\n", AppConfig::supersample);
}

bool OnOff(const char* v, bool& out) {
    if (!std::strcmp(v, "on")  || !std::strcmp(v, "1") || !std::strcmp(v, "true"))  { out = true;  return true; }
    if (!std::strcmp(v, "off") || !std::strcmp(v, "0") || !std::strcmp(v, "false")) { out = false; return true; }
    return false;
}

// Returns false if the command line is malformed (message already printed).
bool ParseArgs(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "bench: %s needs a value\n", name);
                return nullptr;
            }
            return argv[++i];
        };
        if (a == "--help" || a == "-h") { Usage(); std::exit(0); }
        else if (a == "--model")   { const char* v = need("--model");   if (!v) return false; o.model = v; }
        else if (a == "--mode")    { const char* v = need("--mode");    if (!v) return false; o.mode = v; }
        else if (a == "--tag")     { const char* v = need("--tag");     if (!v) return false; o.tag = v; }
        else if (a == "--frames")  { const char* v = need("--frames");  if (!v) return false; o.frames = std::atoi(v); }
        else if (a == "--warmup")  { const char* v = need("--warmup");  if (!v) return false; o.warmup = std::atoi(v); }
        else if (a == "--width")   { const char* v = need("--width");   if (!v) return false; o.width  = std::atoi(v); }
        else if (a == "--height")  { const char* v = need("--height");  if (!v) return false; o.height = std::atoi(v); }
        else if (a == "--repeat")  { const char* v = need("--repeat");  if (!v) return false; o.repeat = std::atoi(v); }
        else if (a == "--spin")    { const char* v = need("--spin");    if (!v) return false; o.spin = (float)std::atof(v); }
        else if (a == "--zoom")    { const char* v = need("--zoom");    if (!v) return false; o.zoom = (float)std::atof(v); }
        else if (a == "--start-angle") { const char* v = need("--start-angle"); if (!v) return false; o.start_angle = (float)std::atof(v); }
        else if (a == "--sweep")   { o.sweep = true; }
        else if (a == "--dump-frames") { const char* v = need("--dump-frames"); if (!v) return false; o.dump_frames = v; }
        else if (a == "--axis") {
            const char* v = need("--axis"); if (!v) return false;
            std::string s2 = v;
            if (s2 != "x" && s2 != "y" && s2 != "z" && s2 != "xz") {
                std::fprintf(stderr, "bench: --axis takes x|y|z|xz\n"); return false;
            }
            o.axis = s2;
        }
        else if (a == "--ssaa")    { const char* v = need("--ssaa");    if (!v) return false; AppConfig::supersample = std::atoi(v); }
        else if (a == "--count-allocs") o.count_allocs = true;
        else if (a == "--raster-stats") o.raster_stats = true;
        else if (a == "--chunks-per-thread") { const char* v = need("--chunks-per-thread"); if (!v) return false; o.chunks_per_thread = std::atoi(v); }
        else if (a == "--csv")          o.csv = true;
        else if (a == "--shading") {
            const char* v = need("--shading"); if (!v) return false;
            std::string s = v;
            if      (s == "none")    Lighting::mode = Lighting::NONE;
            else if (s == "flat")    Lighting::mode = Lighting::FLAT;
            else if (s == "gouraud") Lighting::mode = Lighting::GOURAUD;
            else if (s == "phong")   Lighting::mode = Lighting::PHONG;
            else { std::fprintf(stderr, "bench: unknown shading '%s'\n", v); return false; }
        }
        else if (a == "--par") {
            const char* v = need("--par"); if (!v) return false;
            std::string s = v;
            if (s == "tbb") {
                if (!AppConfig::tbb_available) {
                    std::fprintf(stderr, "bench: --par tbb needs a build with TBB (this one has none)\n");
                    return false;
                }
                AppConfig::use_tbb = true;
            }
            else if (s == "native") AppConfig::use_tbb = false;
            else { std::fprintf(stderr, "bench: --par takes tbb|native\n"); return false; }
        }
        else if (a == "--clip") {
            const char* v = need("--clip"); if (!v) return false;
            std::string s = v;
            if      (s == "lb") AppConfig::clipping_mode = 0;
            else if (s == "cs") AppConfig::clipping_mode = 1;
            else { std::fprintf(stderr, "bench: --clip takes lb|cs\n"); return false; }
        }
        else if (a == "--3d")          { const char* v = need("--3d");          if (!v || !OnOff(v, AppConfig::is3d))          return false; }
        else if (a == "--perspective") { const char* v = need("--perspective"); if (!v || !OnOff(v, AppConfig::perspective))   return false; }
        else if (a == "--zbuffer")     { const char* v = need("--zbuffer");     if (!v || !OnOff(v, AppConfig::z_buffer))      return false; }
        else if (a == "--cull")        { const char* v = need("--cull");        if (!v || !OnOff(v, AppConfig::backface_cull)) return false; }
        else if (a == "--depth-sort")  { const char* v = need("--depth-sort");  if (!v || !OnOff(v, AppConfig::depth_sort))    return false; }
        else {
            std::fprintf(stderr, "bench: unknown option '%s' (try --help)\n", a.c_str());
            return false;
        }
    }
    if (AppConfig::supersample < 1) AppConfig::supersample = 1;
    if (o.frames < 1) o.frames = 1;
    if (o.warmup < 0) o.warmup = 0;
    if (o.zoom <= 0.0f) o.zoom = 1.0f;
    // One full turn spread over the measured frames, so every orientation is
    // sampled exactly once and the worst angle cannot be missed by luck.
    if (o.sweep) o.spin = 360.0f / (float)o.frames;
    return true;
}

// ─── Scene setup ─────────────────────────────────────────────────────────────

struct SceneStats {
    std::size_t objects = 0, vertices = 0, triangles = 0, lines = 0;
    core::Point bb_min, bb_max, center;
    float extent = 1.0f;
};

SceneStats MeasureScene(const std::vector<core::Object>& objs) {
    SceneStats s;
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    for (const auto& o : objs) {
        s.objects++;
        s.vertices  += o.mesh->vertices.size();
        s.triangles += o.mesh->tri_indices.size();
        s.lines     += o.mesh->line_indices.size();
        for (const auto& v : o.mesh->vertices) {
            core::Point w = o.transform * v;   // transform is identity right after import
            const float p[3] = { w.x, w.y, w.z };
            for (int k = 0; k < 3; ++k) { if (p[k] < lo[k]) lo[k] = p[k]; if (p[k] > hi[k]) hi[k] = p[k]; }
        }
    }
    if (s.vertices == 0) { lo[0] = lo[1] = lo[2] = hi[0] = hi[1] = hi[2] = 0.0f; }
    s.bb_min = core::Point(lo[0], lo[1], lo[2]);
    s.bb_max = core::Point(hi[0], hi[1], hi[2]);
    s.center = core::Point((lo[0] + hi[0]) * 0.5f, (lo[1] + hi[1]) * 0.5f, (lo[2] + hi[2]) * 0.5f);
    float e = hi[0] - lo[0];
    e = (hi[1] - lo[1]) > e ? (hi[1] - lo[1]) : e;
    e = (hi[2] - lo[2]) > e ? (hi[2] - lo[2]) : e;
    s.extent = e > 1e-6f ? e : 1.0f;
    return s;
}

// Frame the model so it fills most of the viewport. This is deliberately a
// stress framing and not a "nice" one: geometry that lands outside the window is
// cheap (the clipper drops it early), so a benchmark that half-misses the model
// would under-report the rasterizer.
void FitView(Window& window, const SceneStats& s, float zoom) {
    // The auto-fit is what the user gets after scrolling until the model fills the
    // viewport; `zoom` goes past that, which is a real workload change and not a
    // cosmetic one — closer means more covered pixels per triangle and more
    // geometry straddling the clip window.
    const float fit = s.extent * 1.4f / zoom;
    if (AppConfig::is3d) {
        window.camera.vrp         = s.center;
        window.camera.view_width  = fit;
        window.camera.view_height = fit;
        // Keep the COP a few model-diameters back: with the default 1000 the
        // perspective divide is nearly a no-op on a small model, and on a large
        // one the near plane would sit inside the geometry.
        window.camera.focal_distance = s.extent * 3.0f;   // independent of zoom: the COP
                                                          // stays put, the view volume narrows
        // Rebuilds the NCS matrix from the camera we just edited. Named for the
        // GUI's perspective toggle, but it is the public "recompute now" entry.
        window.OnPerspectiveChanged();
    } else {
        float h = fit * 0.5f;
        window.setWindowBounds(core::Point(s.center.x - h, s.center.y - h, 0.0f),
                               core::Point(s.center.x + h, s.center.y + h, 0.0f));
    }
}

// The pose an animating object is given at angle `a`. "xz" composes a turn about
// X with a turn about Z, which is what models/donut_spin.txt does — and the
// composition matters: a single rotation about (1,0,1) sweeps a different set of
// orientations, so it would sample a different set of costs.
core::mat4 SpinMatrix(float a, const std::string& axis, const core::Point& c) {
    core::mat4 R(true);
    if      (axis == "x") R = core::getRotationMatrixX(a);
    else if (axis == "y") R = core::getRotationMatrixY(a);
    else if (axis == "z") R = core::getRotationMatrixZ(a);
    else                  R = core::getRotationMatrixZ(a) * core::getRotationMatrixX(a);
    return core::getTranslationMatrix(c.x, c.y, c.z) * R
         * core::getTranslationMatrix(-c.x, -c.y, -c.z);
}

// ─── Pipeline mirrors ────────────────────────────────────────────────────────

// MIRRORS: Renderer::DrawObject (src/graphics/Renderer.cpp).
void DrawObjectMirror(Framebuffer& fb, const RenderedObject& obj, int y_lo, int y_hi) {
    const ImU32 col = obj.color;
    const int ss    = AppConfig::supersample;
    const bool zt   = AppConfig::z_buffer;
    const bool less = !AppConfig::depth_ascending;

    if (obj.type == core::ObjectType::POINT) {
        if (!obj.mesh.vertices.empty()) {
            const auto& v = obj.mesh.vertices[0];
            DrawPoint(fb, v.x, v.y, v.z, col, ss, zt, less, y_lo, y_hi);
        }
        return;
    }

    for (const auto& [i, j] : obj.mesh.line_indices) {
        const auto& a = obj.mesh.vertices[i];
        const auto& b = obj.mesh.vertices[j];
        const float lo_y = (a.y < b.y ? a.y : b.y);
        const float hi_y = (a.y < b.y ? b.y : a.y) + (float)ss;
        if (hi_y < (float)y_lo || lo_y >= (float)y_hi) continue;
        DrawLine(fb, a.x, a.y, a.z, b.x, b.y, b.z, col, ss, zt, less, y_lo, y_hi);
    }
}

// Mirrors the band count the application uses. 0 means "whatever
// cg_parallel_chunks_balanced defaults to", which is what the app does; any other
// value overrides K so the tuning can be re-measured on another machine or scene
// without touching the renderer. K=1 reproduces the old one-band-per-thread
// behaviour, which is the A/B baseline.
unsigned g_chunks_per_thread = 0;

// MIRRORS: Renderer::RasterizeFramebuffer (src/graphics/Renderer.cpp).
void RasterizeMirror(Framebuffer& fb,
                     const std::vector<RenderedObject>& drawObjects,
                     const std::vector<SortedTri>& sortedTris,
                     const std::vector<TriBounds>& triBounds,
                     const ShadingContext& shadeCtx) {
    const int H = fb.Height();
    if (H <= 0 || fb.Width() <= 0) return;

    const bool zt   = AppConfig::z_buffer;
    const bool less = !AppConfig::depth_ascending;
    const float farZ = less ?  std::numeric_limits<float>::infinity()
                            : -std::numeric_limits<float>::infinity();

    // MIRRORS the application exactly: same lambda, same balanced scheduler, same
    // min_chunk. Only K is overridable, for re-tuning.
    auto band = [&](std::size_t lo, std::size_t hi) {
        int y_lo = (int)lo, y_hi = (int)hi;
        fb.ClearRows(y_lo, y_hi, 0u);
        if (zt) fb.ClearDepthRows(y_lo, y_hi, farZ);

        for (std::size_t i = 0, n = sortedTris.size(); i < n; ++i) {
            const TriBounds& bb = triBounds[i];
            if (bb.max_y < y_lo || bb.min_y >= y_hi) continue;
            const SortedTri& t = sortedTris[i];
            DrawTriangleFilled(fb, t.a, t.b, t.c, t.za, t.zb, t.zc,
                               bb.min_x, bb.min_y, bb.max_x, bb.max_y,
                               t.P, t.N, t.mat, t.color, shadeCtx, zt, less, y_lo, y_hi);
        }

        for (const auto& obj : drawObjects)
            DrawObjectMirror(fb, obj, y_lo, y_hi);
    };

    cg_parallel_chunks_balanced((std::size_t)H, band,
                                g_chunks_per_thread ? g_chunks_per_thread
                                                    : kDefaultChunksPerThread,
                                /*min_chunk rows=*/8);
}

// One untimed analysis pass over the same work the rasterizer does, counting what
// the inner loop actually costs. Replicates DrawTriangleFilled's bbox clamp and
// inside test WITHOUT drawing, so it can report:
//   - draw calls: (triangle, band) pairs, i.e. how many times the per-triangle
//     setup cost (prologue, area, invArea, clamps) is paid
//   - bbox pixels: total inner-loop iterations
//   - covered pixels: iterations that actually produced a fragment
// It uses the SAME partitioning as RasterizeMirror, so the per-band histogram
// shows the bands the renderer really creates.
struct RasterStats {
    unsigned long long tris = 0, draw_calls = 0, bbox_px = 0, covered_px = 0;
    // Per-band inner-loop iterations, in band order. The frame cannot finish
    // before its busiest band, so the spread here is a direct measure of how much
    // of the available parallelism is actually being used.
    std::vector<unsigned long long> band_px;
};

RasterStats AnalyzeRaster(const Framebuffer& fb,
                          const std::vector<SortedTri>& sortedTris,
                          const std::vector<TriBounds>& triBounds) {
    const int H = fb.Height(), W = fb.Width();
    RasterStats total;
    if (H <= 0 || W <= 0) return total;
    total.tris = sortedTris.size();

    struct BandWork { int y_lo; unsigned long long px, cov, calls; };
    std::vector<BandWork> bands;
    std::mutex bands_mtx;

    auto edge = [](const ImVec2& a, const ImVec2& b, float px, float py) {
        return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
    };

    cg_parallel_chunks_balanced((std::size_t)H, [&](std::size_t lo, std::size_t hi) {
        RasterStats acc;
        const int y_lo = (int)lo, y_hi = (int)hi;
        for (std::size_t i = 0, n = sortedTris.size(); i < n; ++i) {
            const TriBounds& bb = triBounds[i];
            if (bb.max_y < y_lo || bb.min_y >= y_hi) continue;
            const SortedTri& t = sortedTris[i];
            int minx = std::max(bb.min_x, 0), maxx = std::min(bb.max_x, W - 1);
            int miny = std::max(bb.min_y, y_lo), maxy = std::min(bb.max_y, y_hi - 1);
            if (minx > maxx || miny > maxy) continue;
            float area = edge(t.a, t.b, t.c.x, t.c.y);
            if (area == 0.0f) continue;
            acc.draw_calls++;
            acc.bbox_px += (unsigned long long)(maxx - minx + 1) * (maxy - miny + 1);
            for (int y = miny; y <= maxy; ++y) {
                float py = (float)y + 0.5f;
                for (int x = minx; x <= maxx; ++x) {
                    float px = (float)x + 0.5f;
                    float w0 = edge(t.b, t.c, px, py);
                    float w1 = edge(t.c, t.a, px, py);
                    float w2 = edge(t.a, t.b, px, py);
                    if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0))
                        acc.covered_px++;
                }
            }
        }
        std::lock_guard<std::mutex> g(bands_mtx);
        bands.push_back({ y_lo, acc.bbox_px, acc.covered_px, acc.draw_calls });
    }, g_chunks_per_thread ? g_chunks_per_thread : kDefaultChunksPerThread,
       /*min_chunk rows=*/8);

    std::sort(bands.begin(), bands.end(),
              [](const BandWork& a, const BandWork& b) { return a.y_lo < b.y_lo; });
    for (const auto& b : bands) {
        total.draw_calls += b.calls;
        total.bbox_px    += b.px;
        total.covered_px += b.cov;
        total.band_px.push_back(b.px);
    }
    return total;
}

// MIRRORS: the per-frame shading block in Renderer::render(). Rebuilt every
// frame in the application (lights are read live, not cached), so it is rebuilt
// every frame here too.
void BuildShadingContext(const Window& window,
                         std::vector<core::Light>& effectiveLights,
                         ShadingContext& shadeCtx) {
    effectiveLights.clear();
    for (const auto& L : Lighting::lights) effectiveLights.push_back(L);
    if (Lighting::headlight) {
        core::Light hl;
        hl.position  = window.GetEyeWorld();
        hl.color     = Lighting::headlight_color;
        hl.intensity = Lighting::headlight_intensity;
        effectiveLights.push_back(hl);
    }
    shadeCtx.mode    = Lighting::mode;
    shadeCtx.eye     = window.GetEyeWorld();
    shadeCtx.ambient = Lighting::ambient;
    shadeCtx.lights  = &effectiveLights;
}

// FNV-1a over the resolved framebuffer. A scheduling change (which thread draws
// which band) must not alter a single byte; a maths change (the incremental edge
// functions, later) legitimately can. Printing the digest every run turns "it
// looks the same" into something checkable.
unsigned long long FrameDigest(const Framebuffer& fb) {
    unsigned long long h = 1469598103934665603ull;
    for (ImU32 p : fb.ResolvedPixels()) {
        for (int b = 0; b < 4; ++b) {
            h ^= (unsigned long long)((p >> (b * 8)) & 0xFFu);
            h *= 1099511628211ull;
        }
    }
    return h;
}

// ─── Reporting ───────────────────────────────────────────────────────────────

void ReportAllocs(const bench::AllocSnapshot& s, int frames, const char* unit) {
    std::printf("  allocations: %llu new / %llu delete, %.1f MB requested\n",
                (unsigned long long)s.allocs, (unsigned long long)s.frees,
                (double)s.bytes / (1024.0 * 1024.0));
    if (frames > 0) {
        std::printf("               %.0f new per %s, %.1f KB per %s\n",
                    (double)s.allocs / frames, unit,
                    (double)s.bytes / frames / 1024.0, unit);
    }
}

} // namespace

// ─── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    Options opt;
    if (!ParseArgs(argc, argv, opt)) return 2;

    // The real application objects. Nothing here touches GLFW or an ImGui
    // context: Viewport only holds the canvas rectangle (fed below instead of by
    // ImGui), and Renderer is constructed only because EntityManager needs one
    // to notify — its render() is never called.
    ExampleAppLog log;
    Viewport      viewport(log);
    Window        window(viewport);
    DisplayFile   displayFile;
    Renderer      renderer(displayFile, viewport, window, log);
    EntityManager entityManager(displayFile, renderer);

    // Feed the canvas rectangle the GUI would have measured from ImGui.
    viewport.SetCanvas(ImVec2(0.0f, 0.0f), ImVec2((float)opt.width, (float)opt.height));

    // ── Import ──────────────────────────────────────────────────────────────
    mtl::MaterialLibrary lib;
    std::string mtl_path = opt.model.substr(0, opt.model.find_last_of('.')) + ".mtl";
    { std::FILE* f = std::fopen(mtl_path.c_str(), "r"); if (f) { std::fclose(f); mtl::Load(mtl_path, lib); } }

    if (opt.mode == "import") {
        std::printf("=== import: %s x%d ===\n", opt.model.c_str(), opt.repeat);
        bench::Stage st("import");
        if (opt.count_allocs) { bench::AllocReset(); bench::AllocCountingEnable(true); }
        for (int i = 0; i < opt.repeat; ++i) {
            entityManager.removeAll();
            mtl::MaterialLibrary l2;
            { std::FILE* f = std::fopen(mtl_path.c_str(), "r"); if (f) { std::fclose(f); mtl::Load(mtl_path, l2); } }
            bench::Timer t;
            obj::ImportResult r = obj::Import(opt.model, l2, entityManager);
            st.add(t.ms());
            if (!r.error.empty()) { std::fprintf(stderr, "bench: %s\n", r.error.c_str()); return 1; }
        }
        bench::AllocCountingEnable(false);
        std::vector<bench::Stage> stages{ st };
        if (opt.csv) bench::PrintCSV(stages, opt.tag);
        else {
            SceneStats s = MeasureScene(entityManager.getObjects());
            std::printf("  %zu objects, %zu vertices, %zu triangles\n",
                        s.objects, s.vertices, s.triangles);
            bench::PrintTable(stages, opt.repeat, "import");
            if (opt.count_allocs) ReportAllocs(bench::AllocRead(), opt.repeat, "import");
        }
        return 0;
    }

    bench::Timer import_timer;
    obj::ImportResult ir = obj::Import(opt.model, lib, entityManager);
    double import_ms = import_timer.ms();
    if (!ir.error.empty()) {
        std::fprintf(stderr, "bench: failed to import '%s': %s\n",
                     opt.model.c_str(), ir.error.c_str());
        return 1;
    }
    if (entityManager.GetObjectCount() == 0) {
        std::fprintf(stderr, "bench: '%s' produced no objects\n", opt.model.c_str());
        return 1;
    }

    SceneStats scene = MeasureScene(entityManager.getObjects());
    FitView(window, scene, opt.zoom);

    // ── Header ──────────────────────────────────────────────────────────────
    if (!opt.csv) {
        const char* shading[] = { "none", "flat", "gouraud", "phong" };
        std::printf("\n=== CG headless bench ===\n");
        std::printf("  model        %s  (%.0f ms to import)\n", opt.model.c_str(), import_ms);
        std::printf("  scene        %zu objects, %zu vertices, %zu triangles, %zu lines\n",
                    scene.objects, scene.vertices, scene.triangles, scene.lines);
        std::printf("  viewport     %dx%d display, SSAA x%d -> %dx%d scene buffer (%.1f MB color + depth)\n",
                    opt.width, opt.height, AppConfig::supersample,
                    opt.width * AppConfig::supersample, opt.height * AppConfig::supersample,
                    (double)opt.width * opt.height * AppConfig::supersample * AppConfig::supersample
                        * 8.0 / (1024.0 * 1024.0));
        std::printf("  mode         %s (%d warmup + %d measured frames, spin %.2f deg/frame about %s%s)\n",
                    opt.mode.c_str(), opt.warmup, opt.frames, opt.spin, opt.axis.c_str(),
                    opt.sweep ? ", one full turn" : "");
        std::printf("  framing      auto-fit x%.2f zoom -> view volume %.5f world units wide\n",
                    opt.zoom, window.camera.view_width);
        std::printf("  config       3d=%s perspective=%s zbuffer=%s cull=%s depth_sort=%s clip=%s shading=%s\n",
                    AppConfig::is3d ? "on" : "off", AppConfig::perspective ? "on" : "off",
                    AppConfig::z_buffer ? "on" : "off", AppConfig::backface_cull ? "on" : "off",
                    AppConfig::depth_sort ? "on" : "off",
                    AppConfig::clipping_mode == 0 ? "liang-barsky" : "cohen-sutherland",
                    shading[Lighting::mode & 3]);
        std::printf("  threads      %u hardware threads, %s scheduler\n",
                    std::thread::hardware_concurrency(),
                    AppConfig::use_tbb ? "TBB"
                                       : (AppConfig::tbb_available ? "native (TBB built in, switched off)"
                                                                   : "native (no TBB in this build)"));
        if (opt.count_allocs)
            std::printf("  NOTE         allocation counting is ON: these timings are inflated by the\n"
                        "               counters and must not be compared against a normal run.\n");
    }

    // ── Per-frame state, owned exactly like Renderer owns it ────────────────
    // These are members of Renderer in the application, kept alive across frames
    // so their capacity is reused. Reproducing that here matters: a benchmark
    // that reallocated them every frame would invent allocation traffic the
    // application does not have.
    std::vector<RenderedObject> drawObjects;
    std::vector<SortedTri>      sortedTris;
    std::vector<TriBounds>      triBounds;
    std::vector<core::Light>    effectiveLights;
    ShadingContext              shadeCtx;
    Framebuffer                 framebuffer;

    framebuffer.Resize(opt.width, opt.height, AppConfig::supersample);
    g_chunks_per_thread = (unsigned)std::max(0, opt.chunks_per_thread);

    std::vector<bench::Stage> stages;
    stages.emplace_back("transform+ncs");
    stages.emplace_back("clip-near");
    stages.emplace_back("project");
    stages.emplace_back("clip-window");
    stages.emplace_back("sort-triangles");
    stages.emplace_back("to-viewport");
    stages.emplace_back("shade-setup");
    stages.emplace_back("rasterize");
    stages.emplace_back("resolve");
    for (auto& s : stages) s.reserve((std::size_t)opt.frames);
    enum { TRANSFORM, CLIPNEAR, PROJECT, CLIPWIN, SORTTRI, TOVIEW, SHADE, RASTER, RESOLVE };

    const std::vector<long long> ids = entityManager.GetObjectIDs();
    const bool raster_only = (opt.mode == "raster");

    // Times one stage and, when --count-allocs is on, attributes the allocations
    // it made to it. The alloc delta is read after the call returns, i.e. after
    // any parallel region inside it has joined, so work done on TBB workers is
    // charged to the stage that spawned it rather than to whoever ran next.
    double cur_frame_ms = 0.0;   // summed by run_stage, harvested at the end of each frame
    auto run_stage = [&](int idx, bool measure, auto&& fn) {
        if (!measure) { fn(); return; }
        const bool counting = bench::AllocCountingEnabled();
        bench::AllocSnapshot a0;
        if (counting) a0 = bench::AllocRead();
        bench::Timer t;
        fn();
        const double ms = t.ms();
        stages[idx].add(ms);
        cur_frame_ms += ms;
        if (counting) {
            bench::AllocSnapshot a1 = bench::AllocRead();
            stages[idx].allocs += a1.allocs - a0.allocs;
            stages[idx].bytes  += a1.bytes  - a0.bytes;
        }
    };

    // Runs the geometry half of the pipeline. MIRRORS: Renderer::GenerateDrawList
    // (ProcessPreClipping -> ApplyClipping -> BuildSortedTriangles ->
    // ApplyViewportTransform), split so each stage is measured separately.
    auto run_geometry = [&](bool measure) {
        if (AppConfig::is3d && AppConfig::perspective) {
            run_stage(TRANSFORM, measure, [&] {
                TransformObjectAndDoNCS(drawObjects, displayFile.getObjects(), window.GetVRCMatrix());
            });
            run_stage(CLIPNEAR, measure, [&] {
                ClipNearPlane(drawObjects, window.GetNearPlaneZ());
            });
            run_stage(PROJECT, measure, [&] {
                ProjectVertices(drawObjects, window.GetProjectionScaleMatrix());
            });
        } else {
            run_stage(TRANSFORM, measure, [&] {
                TransformObjectAndDoNCS(drawObjects, displayFile.getObjects(), window.GetWindowNCSMatrix());
            });
        }

        run_stage(CLIPWIN, measure, [&] {
            auto [clip_min, clip_max] = window.getClipBoundsNCS();
            ClipObjects(drawObjects, clip_min, clip_max, AppConfig::clipping_mode);
        });
        run_stage(SORTTRI, measure, [&] {
            BuildSortedTriangles(drawObjects, window, (float)AppConfig::supersample,
                                 sortedTris, triBounds);
        });
        run_stage(TOVIEW, measure, [&] {
            TransformToViewport(drawObjects, window, (float)AppConfig::supersample);
        });
    };

    // Seed the pose before anything is built, so --mode raster holds the requested
    // orientation rather than the model's authored one.
    if (opt.start_angle != 0.0f) {
        core::mat4 R0 = SpinMatrix(opt.start_angle, opt.axis, scene.center);
        for (long long id : ids) entityManager.SetTransformation(id, R0);
    }

    // In raster mode the geometry is built once and then held, which is what the
    // application does whenever the RendererCache reports no change.
    if (raster_only) run_geometry(false);

    if (opt.count_allocs) { bench::AllocReset(); }

    bench::Stage frame_total("frame total");
    frame_total.reserve((std::size_t)opt.frames);
    std::vector<float> frame_angles;
    frame_angles.reserve((std::size_t)opt.frames);

    float angle = opt.start_angle;
    for (int f = 0; f < opt.warmup + opt.frames; ++f) {
        const bool measure = (f >= opt.warmup);
        if (measure && opt.count_allocs && f == opt.warmup) bench::AllocCountingEnable(true);
        // Restart the sweep at 0 for the first measured frame, so --sweep covers
        // exactly one turn over the frames that are reported and the warmup does
        // not shift which orientations get sampled.
        if (measure && f == opt.warmup) angle = opt.start_angle;
        cur_frame_ms = 0.0;

        if (!raster_only) {
            // MIRRORS: what ObjectController::Update does to an animating object —
            // the full pose is recomputed and assigned, not accumulated.
            if (opt.spin != 0.0f) {
                angle += opt.spin;
                core::mat4 R = SpinMatrix(angle, opt.axis, scene.center);
                for (long long id : ids) entityManager.SetTransformation(id, R);
            }
            run_geometry(measure);
        }

        run_stage(SHADE,  measure, [&] { BuildShadingContext(window, effectiveLights, shadeCtx); });
        run_stage(RASTER, measure, [&] {
            RasterizeMirror(framebuffer, drawObjects, sortedTris, triBounds, shadeCtx);
        });
        run_stage(RESOLVE, measure, [&] { framebuffer.Resolve(); });

        if (measure) {
            frame_total.add(cur_frame_ms);
            frame_angles.push_back(raster_only ? 0.0f : angle);
        }
    }
    bench::AllocCountingEnable(false);

    if (!opt.csv)
        std::printf("  framebuffer digest   %016llx   (must not change across a\n"
                    "                       scheduling-only refactor)\n\n",
                    FrameDigest(framebuffer));

    if (opt.raster_stats && !opt.csv) {
        RasterStats rs = AnalyzeRaster(framebuffer, sortedTris, triBounds);
        const double out_px = (double)framebuffer.Width() * framebuffer.Height();
        std::printf("  rasterizer work for the final frame\n");
        std::printf("  %s\n", std::string(66, '-').c_str());
        std::printf("  %-24s %14llu\n", "triangles submitted", rs.tris);
        std::printf("  %-24s %14llu   (%.2f per triangle)\n", "draw calls (tri x band)",
                    rs.draw_calls, rs.tris ? (double)rs.draw_calls / rs.tris : 0.0);
        std::printf("  %-24s %14llu   (%.1f per draw call)\n", "inner-loop iterations",
                    rs.bbox_px, rs.draw_calls ? (double)rs.bbox_px / rs.draw_calls : 0.0);
        std::printf("  %-24s %14llu   (%.1f%% of iterations hit)\n", "covered fragments",
                    rs.covered_px, rs.bbox_px ? 100.0 * rs.covered_px / rs.bbox_px : 0.0);
        std::printf("  %-24s %14.0f   (overdraw %.2fx)\n", "framebuffer pixels",
                    out_px, out_px ? (double)rs.covered_px / out_px : 0.0);

        if (!rs.band_px.empty()) {
            unsigned long long bmax = 0, bmin = ~0ull, bsum = 0;
            for (auto v : rs.band_px) {
                bmax = std::max(bmax, v);
                bmin = std::min(bmin, v);
                bsum += v;
            }

            std::printf("\n  work per band (%zu bands over %u hardware threads)\n",
                        rs.band_px.size(), std::thread::hardware_concurrency());
            std::printf("  %s\n", std::string(66, '-').c_str());
            // Listing 128 bands is noise; the shape is what matters. Above a couple
            // dozen, fold neighbours into one row so the histogram still fits.
            const std::size_t rows = std::min<std::size_t>(rs.band_px.size(), 24);
            const std::size_t per_row = (rs.band_px.size() + rows - 1) / rows;
            unsigned long long rmax = 0;
            std::vector<unsigned long long> folded;
            for (std::size_t i = 0; i < rs.band_px.size(); i += per_row) {
                unsigned long long v = 0;
                for (std::size_t j = i; j < std::min(i + per_row, rs.band_px.size()); ++j)
                    v += rs.band_px[j];
                folded.push_back(v);
                rmax = std::max(rmax, v);
            }
            for (std::size_t i = 0; i < folded.size(); ++i) {
                int bar = (int)(40.0 * (double)folded[i] / (double)(rmax ? rmax : 1));
                std::printf("  rows %4zu-%4zu %12llu  %s\n",
                            i * per_row, std::min((i + 1) * per_row, rs.band_px.size()) - 1,
                            folded[i], std::string((std::size_t)bar, '#').c_str());
            }
            std::printf("  %s\n", std::string(66, '-').c_str());
            // A band is indivisible: whichever thread picks it up runs it to the
            // end. So the frame cannot beat EITHER a perfect split of the total
            // across the threads, OR the single largest band — whichever is worse.
            // With one band per thread the largest band always dominated, which is
            // why that used to be the whole story; with K bands per thread the
            // total/threads term normally wins and the granularity stops mattering.
            unsigned int nthreads = std::thread::hardware_concurrency();
            if (nthreads == 0) nthreads = 1;
            const double ideal = (double)bsum / nthreads;
            const double floor_ = std::max(ideal, (double)bmax);
            (void)bmin;

            std::printf("  total work           %14llu\n", bsum);
            std::printf("  ideal per thread     %14.0f   (total / %u threads)\n", ideal, nthreads);
            std::printf("  largest single band  %14llu   (%.2fx the ideal share)\n",
                        bmax, ideal > 0 ? (double)bmax / ideal : 0.0);
            std::printf("  achievable floor     %14.0f   -> at best %.0f%% of the\n"
                        "                                      parallelism is usable\n",
                        floor_, floor_ > 0 ? 100.0 * ideal / floor_ : 0.0);
            if ((double)bmax > ideal)
                std::printf("  LIMITED BY GRANULARITY: one band exceeds an even share, so some\n"
                            "  thread must run it alone. Smaller bands (higher K) would help.\n\n");
            else
                std::printf("  granularity is fine: no band exceeds an even share, so the\n"
                            "  scheduler can distribute the work evenly.\n\n");
        }
    }

    if (opt.csv) {
        std::vector<bench::Stage> with_total = stages;
        with_total.push_back(frame_total);
        bench::PrintCSV(with_total, opt.tag);
    } else {
        bench::PrintTable(stages, opt.frames);
        bench::PrintFrameSummary(frame_total, raster_only ? std::vector<float>{} : frame_angles);
        if (opt.count_allocs) {
            bench::PrintAllocTable(stages, opt.frames);
            ReportAllocs(bench::AllocRead(), opt.frames, "frame");
        }
    }

    if (!opt.dump_frames.empty()) {
        if (bench::DumpFrames(opt.dump_frames, stages, frame_total,
                              raster_only ? std::vector<float>{} : frame_angles))
            std::fprintf(stderr, "bench: wrote %s\n", opt.dump_frames.c_str());
        else
            std::fprintf(stderr, "bench: could not write %s\n", opt.dump_frames.c_str());
    }

    // Framebuffer::Present is never called, so no GL texture was ever created and
    // ~Framebuffer has nothing to release. Stated explicitly because a GL delete
    // without a context would be a crash, not a leak.
    return 0;
}
