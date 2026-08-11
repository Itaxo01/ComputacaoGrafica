#include "RendererClipping.hpp"
#include "ParallelUtils.hpp"
#include <atomic>
#include <mutex>
#include <vector>
#include <cmath>

// ── Region-code helpers (Cohen-Sutherland) ────────────────────────────────────

typedef int OUT;
const int INSIDE = 0b0000;
const int LEFT   = 0b0001;
const int RIGHT  = 0b0010;
const int BOTTOM = 0b0100;
const int TOP    = 0b1000;

static inline OUT ComputeOut(const core::Point& p, const core::Point& wp0, const core::Point& wp1) {
    OUT ret = INSIDE;
    if      (p.x < wp0.x) ret |= LEFT;
    else if (p.x > wp1.x) ret |= RIGHT;
    if      (p.y < wp0.y) ret |= BOTTOM;
    else if (p.y > wp1.y) ret |= TOP;
    return ret;
}

// ── Liang-Barsky helpers ──────────────────────────────────────────────────────

static inline bool LineClipTest(float p, float q, float& u1, float& u2) {
    if (p == 0.0f) {
        if (q < 0.0f) return false;
    } else {
        float r = q / p;
        if (p < 0.0f) { if (r > u2) return false; if (r > u1) u1 = r; }
        else           { if (r < u1) return false; if (r < u2) u2 = r; }
    }
    return true;
}

static bool ClipSegmentLiangBarsky(core::Point& a, core::Point& b,
                                    const core::Point& wp0, const core::Point& wp1) {
    float u1 = 0.0f, u2 = 1.0f;
    float dx = b.x - a.x, dy = b.y - a.y;
    if (!LineClipTest(-dx, a.x - wp0.x, u1, u2)) return false;
    if (!LineClipTest( dx, wp1.x - a.x, u1, u2)) return false;
    if (!LineClipTest(-dy, a.y - wp0.y, u1, u2)) return false;
    if (!LineClipTest( dy, wp1.y - a.y, u1, u2)) return false;
    float ox = a.x, oy = a.y;
    if (u1 > 0.0f) { a.x = ox + u1*dx; a.y = oy + u1*dy; }
    if (u2 < 1.0f) { b.x = ox + u2*dx; b.y = oy + u2*dy; }
    return true;
}

static bool ClipSegmentCohenSutherland(core::Point& a, core::Point& b,
                                        const core::Point& wp0, const core::Point& wp1) {
    OUT out1 = ComputeOut(a, wp0, wp1);
    OUT out2 = ComputeOut(b, wp0, wp1);
    while (true) {
        if (!(out1 | out2)) return true;
        if (out1 & out2)    return false;
        float x = 0, y = 0;
        OUT oc = out2 > out1 ? out2 : out1;
        if      (oc & TOP)    { x = a.x + (b.x-a.x)*(wp1.y-a.y)/(b.y-a.y); y = wp1.y; }
        else if (oc & BOTTOM) { x = a.x + (b.x-a.x)*(wp0.y-a.y)/(b.y-a.y); y = wp0.y; }
        else if (oc & RIGHT)  { y = a.y + (b.y-a.y)*(wp1.x-a.x)/(b.x-a.x); x = wp1.x; }
        else if (oc & LEFT)   { y = a.y + (b.y-a.y)*(wp0.x-a.x)/(b.x-a.x); x = wp0.x; }
        if (oc == out1) { a.x = x; a.y = y; out1 = ComputeOut(a, wp0, wp1); }
        else            { b.x = x; b.y = y; out2 = ComputeOut(b, wp0, wp1); }
    }
}

// ── ClipLine (public — used by RendererBackground) ────────────────────────────

bool ClipLine(core::Line& line, const core::Point& wp0, const core::Point& wp1) {
    return ClipSegmentLiangBarsky(line.a, line.b, wp0, wp1);
}

// ── Sutherland-Hodgman polygon clipping (attribute-aware) ─────────────────────

// A polygon vertex that carries the shading attributes alongside the clip-space
// position, so they get interpolated at every clip intersection (keeping them
// index-aligned with the geometry after clipping). When shading is off, world/
// normal are just unused zeros riding along.
struct ClipVert {
    core::Point pos;     // NCS position (x/y clipped against the window; z carried)
    core::Point world;   // world-space position (Phong)
    core::Point normal;  // world-space normal (Phong)
};

static inline ClipVert lerpCV(const ClipVert& a, const ClipVert& b, float t) {
    return { a.pos    + (b.pos    - a.pos)    * t,
             a.world  + (b.world  - a.world)  * t,
             a.normal + (b.normal - a.normal) * t };
}

// Clipping a convex polygon against one half-plane adds at most one vertex, so a
// triangle can never exceed 3 + 4 = 7 vertices after the four window edges. The
// buffer is sized past that bound and every write is guarded, so a degenerate
// input that produces extra crossings through float error degrades (drops a
// vertex) instead of running off the end.
static constexpr int kMaxClipVerts = 12;

// One window edge as a half-plane on a single axis: keep coord >= limit
// (keep_greater) or coord <= limit.
struct ClipPlane { bool y_axis; bool keep_greater; float limit; };

// Sutherland-Hodgman over a fixed-capacity buffer. `poly` holds `n` vertices on
// entry and the clipped polygon on return (the new count). Replaces the old
// std::vector version, which allocated on every window edge of every triangle.
static int SHClipFixed(ClipVert* poly, int n,
                       const core::Point& wp0, const core::Point& wp1) {
    const ClipPlane planes[4] = {
        { false, true,  wp0.x }, { false, false, wp1.x },
        { true,  true,  wp0.y }, { true,  false, wp1.y },
    };

    ClipVert scratch[kMaxClipVerts];
    for (const ClipPlane& pl : planes) {
        if (n == 0) return 0;

        auto coord  = [&](const ClipVert& v) { return pl.y_axis ? v.pos.y : v.pos.x; };
        auto inside = [&](float c) { return pl.keep_greater ? (c >= pl.limit) : (c <= pl.limit); };

        int m = 0;
        const ClipVert* prev = &poly[n - 1];
        float cprev = coord(*prev);
        bool  iprev = inside(cprev);

        for (int i = 0; i < n; ++i) {
            const ClipVert& cur = poly[i];
            const float ccur = coord(cur);
            const bool  icur = inside(ccur);
            // Crossing the edge: one endpoint in, one out, so the denominator on
            // this axis is nonzero and the parametric hit is well defined.
            if (icur != iprev && m < kMaxClipVerts)
                scratch[m++] = lerpCV(*prev, cur, (pl.limit - cprev) / (ccur - cprev));
            if (icur && m < kMaxClipVerts)
                scratch[m++] = cur;
            prev = &cur; cprev = ccur; iprev = icur;
        }

        n = m;
        for (int i = 0; i < n; ++i) poly[i] = scratch[i];
    }
    return n;
}

// ── Per-thread output bucket ──────────────────────────────────────────────────

// Clipped triangle output from one thread. Triangle indices are LOCAL to this
// bucket's `verts`; MergeBuckets rebases them when it splices the buckets into
// the object's final arrays. Output order is irrelevant (the painter's sort
// reorders, and the z-buffer resolves visibility per pixel).
struct ClipBucket {
    std::vector<core::Point> verts, world, normals;
    std::vector<std::tuple<uint32_t,uint32_t,uint32_t>> tris;
};

// Splices `buckets` onto the tail of the object's arrays. `vert_base` is where
// the first spliced vertex lands, i.e. the count of vertices already present
// (the clipped wireframe verts, which come first).
static void MergeBuckets(std::vector<ClipBucket>& buckets, bool shaded, uint32_t vert_base,
                         std::vector<core::Point>& verts,
                         std::vector<core::Point>& world,
                         std::vector<core::Point>& normals,
                         std::vector<std::tuple<uint32_t,uint32_t,uint32_t>>& tris) {
    size_t nv = 0, nt = 0;
    for (const auto& b : buckets) { nv += b.verts.size(); nt += b.tris.size(); }
    verts.reserve(verts.size() + nv);
    tris.reserve(tris.size() + nt);
    if (shaded) {
        world.reserve(world.size() + nv);
        normals.reserve(normals.size() + nv);
    }

    uint32_t off = vert_base;
    for (auto& b : buckets) {
        verts.insert(verts.end(), b.verts.begin(), b.verts.end());
        if (shaded) {
            world.insert(world.end(), b.world.begin(), b.world.end());
            normals.insert(normals.end(), b.normals.begin(), b.normals.end());
        }
        for (const auto& [i, j, k] : b.tris)
            tris.emplace_back(off + i, off + j, off + k);
        off += (uint32_t)b.verts.size();
    }
}

// Runs `body(lo, hi, bucket)` over the triangle range [0, n): split across
// threads into private buckets when the mesh is big enough to pay for it,
// otherwise straight into a single bucket on this thread. Mirrors the
// per-triangle split BuildSortedTriangles already uses.
namespace { constexpr std::size_t kTriangleParallelThreshold = 16384; }

template <typename Body>
static void GatherTriangles(std::size_t n, Body&& body, std::vector<ClipBucket>& buckets) {
    if (n >= kTriangleParallelThreshold) {
        std::mutex mtx;
        cg_parallel_chunks(n, [&](std::size_t lo, std::size_t hi) {
            ClipBucket local;
            local.verts.reserve((hi - lo) * 3);
            local.tris.reserve(hi - lo);
            body(lo, hi, local);
            std::lock_guard<std::mutex> g(mtx);
            buckets.push_back(std::move(local));   // ~one lock per thread
        });
    } else {
        buckets.emplace_back();
        ClipBucket& local = buckets.back();
        local.verts.reserve(n * 3);
        local.tris.reserve(n);
        body(0, n, local);
    }
}

// ── Near-plane clip (VRC space, before the perspective divide) ────────────────

// Clip segment a→b against the plane z = near_z, keeping the z >= near_z side.
// Returns false if the whole segment is behind the plane.
static bool ClipSegmentNear(core::Point& a, core::Point& b, float near_z) {
    float da = a.z - near_z;
    float db = b.z - near_z;
    if (da < 0.0f && db < 0.0f) return false;   // both behind the COP
    if (da >= 0.0f && db >= 0.0f) return true;  // both in front
    // Crossing: move the behind endpoint to the intersection point.
    float t = da / (da - db);
    core::Point hit = { a.x + t * (b.x - a.x),
                        a.y + t * (b.y - a.y),
                        a.z + t * (b.z - a.z) };
    if (da < 0.0f) a = hit;
    else           b = hit;
    return true;
}

void ClipNearPlane(std::vector<RenderedObject>& objs, float near_z) {
    cg_parallel_for_each_heavy(objs.begin(), objs.end(), [&](RenderedObject& obj) {
        if (obj.type == core::ObjectType::POINT) {
            if (!obj.mesh.vertices.empty() && obj.mesh.vertices[0].z < near_z)
                obj.mesh.vertices.clear();
            return;
        }

        const bool shaded = !obj.mesh.world_vertices.empty();
        // Move the sources out: the outputs are built into fresh arrays and moved
        // back at the end, so nothing here copies a vertex array.
        const std::vector<core::Point> src_verts  = std::move(obj.mesh.vertices);
        const std::vector<core::Point> src_world  = std::move(obj.mesh.world_vertices);
        const std::vector<core::Point> src_normal = std::move(obj.mesh.world_normals);
        const auto src_lines = std::move(obj.mesh.line_indices);
        const auto src_tris  = std::move(obj.mesh.tri_indices);

        std::vector<core::Point> new_verts, new_world, new_normal;
        std::vector<std::pair<uint32_t,uint32_t>> new_lines;
        new_verts.reserve(src_lines.size() * 2);
        new_lines.reserve(src_lines.size());
        for (const auto& [i, j] : src_lines) {
            core::Point a = src_verts[i];
            core::Point b = src_verts[j];
            if (ClipSegmentNear(a, b, near_z)) {
                uint32_t na = (uint32_t)new_verts.size(); new_verts.push_back(a);
                uint32_t nb = (uint32_t)new_verts.size(); new_verts.push_back(b);
                new_lines.push_back({na, nb});
                if (shaded) { // lines aren't shaded — placeholders keep arrays aligned
                    new_world.push_back(core::Point()); new_world.push_back(core::Point());
                    new_normal.push_back(core::Point()); new_normal.push_back(core::Point());
                }
            }
        }

        // Filled triangles: conservative — keep only those fully in front of the
        // near plane (rare in wireframe scenes; avoids re-triangulating here). Kept
        // whole, so shading attributes are copied (no interpolation needed).
        std::vector<std::tuple<uint32_t,uint32_t,uint32_t>> new_tris;
        std::vector<ClipBucket> buckets;
        GatherTriangles(src_tris.size(), [&](std::size_t lo, std::size_t hi, ClipBucket& sink) {
            for (std::size_t idx = lo; idx < hi; ++idx) {
                const auto& [ti, tj, tk] = src_tris[idx];
                const core::Point& A = src_verts[ti];
                const core::Point& B = src_verts[tj];
                const core::Point& C = src_verts[tk];
                if (A.z < near_z || B.z < near_z || C.z < near_z) continue;

                uint32_t base = (uint32_t)sink.verts.size();
                sink.verts.push_back(A);
                sink.verts.push_back(B);
                sink.verts.push_back(C);
                sink.tris.emplace_back(base, base + 1, base + 2);
                if (shaded) {
                    sink.world.push_back(src_world[ti]);
                    sink.world.push_back(src_world[tj]);
                    sink.world.push_back(src_world[tk]);
                    sink.normals.push_back(src_normal[ti]);
                    sink.normals.push_back(src_normal[tj]);
                    sink.normals.push_back(src_normal[tk]);
                }
            }
        }, buckets);

        MergeBuckets(buckets, shaded, (uint32_t)new_verts.size(),
                     new_verts, new_world, new_normal, new_tris);

        obj.mesh.vertices     = std::move(new_verts);
        obj.mesh.line_indices = std::move(new_lines);
        obj.mesh.tri_indices  = std::move(new_tris);
        if (shaded) {
            obj.mesh.world_vertices = std::move(new_world);
            obj.mesh.world_normals  = std::move(new_normal);
        }
    });
}

void ClipObjects(std::vector<RenderedObject>& objs,
                 const core::Point& wp0, const core::Point& wp1,
                 int line_clip_mode) {
    cg_parallel_for_each_heavy(objs.begin(), objs.end(), [&](auto& obj) {
        const bool has_fill = obj.type == core::ObjectType::POLYGON ||
                              obj.type == core::ObjectType::MESH ||
                              obj.type == core::ObjectType::SURFACE;
        if (has_fill && obj.filled) {
            // Sources are moved out (not copied) and the results are built into
            // fresh arrays; at 300k faces the old copies alone were several MB
            // of pointless traffic per rebuild.
            const std::vector<core::Point> orig_verts  = std::move(obj.mesh.vertices);
            const std::vector<core::Point> orig_world  = std::move(obj.mesh.world_vertices);
            const std::vector<core::Point> orig_normal = std::move(obj.mesh.world_normals);
            const auto orig_tri_idx = std::move(obj.mesh.tri_indices);
            const auto orig_lines   = std::move(obj.mesh.line_indices);
            const bool shaded = !orig_world.empty();

            // Wireframe edges (Liang-Barsky). These verts come first in the array;
            // shading only reads tri-vertex indices, so they get attribute placeholders.
            std::vector<core::Point> new_verts;
            std::vector<std::pair<uint32_t,uint32_t>> new_lines;
            new_verts.reserve(orig_lines.size() * 2);
            new_lines.reserve(orig_lines.size());
            for (const auto& [i, j] : orig_lines) {
                core::Point a = orig_verts[i];
                core::Point b = orig_verts[j];
                if (ClipSegmentLiangBarsky(a, b, wp0, wp1)) {
                    uint32_t na = (uint32_t)new_verts.size(); new_verts.push_back(a);
                    uint32_t nb = (uint32_t)new_verts.size(); new_verts.push_back(b);
                    new_lines.push_back({na, nb});
                }
            }
            const uint32_t line_count = (uint32_t)new_verts.size();

            std::vector<std::tuple<uint32_t,uint32_t,uint32_t>> new_tris;
            std::vector<ClipBucket> buckets;
            GatherTriangles(orig_tri_idx.size(),
                            [&](std::size_t lo, std::size_t hi, ClipBucket& sink) {
                for (std::size_t idx = lo; idx < hi; ++idx) {
                    const auto& [ti, tj, tk] = orig_tri_idx[idx];
                    const core::Point& A = orig_verts[ti];
                    const core::Point& B = orig_verts[tj];
                    const core::Point& C = orig_verts[tk];

                    // Region codes give the two cheap outcomes first. In a typical
                    // view almost every triangle is wholly inside or wholly outside,
                    // so the general clip below runs only on the window border.
                    const OUT oa = ComputeOut(A, wp0, wp1);
                    const OUT ob = ComputeOut(B, wp0, wp1);
                    const OUT oc = ComputeOut(C, wp0, wp1);
                    if (oa & ob & oc) continue;    // all outside the same edge: reject

                    const uint32_t base = (uint32_t)sink.verts.size();

                    if (!(oa | ob | oc)) {         // wholly inside: pass through as-is
                        sink.verts.push_back(A);
                        sink.verts.push_back(B);
                        sink.verts.push_back(C);
                        if (shaded) {
                            sink.world.push_back(orig_world[ti]);
                            sink.world.push_back(orig_world[tj]);
                            sink.world.push_back(orig_world[tk]);
                            sink.normals.push_back(orig_normal[ti]);
                            sink.normals.push_back(orig_normal[tj]);
                            sink.normals.push_back(orig_normal[tk]);
                        }
                        sink.tris.emplace_back(base, base + 1, base + 2);
                        continue;
                    }

                    ClipVert poly[kMaxClipVerts] = {
                        { A, shaded ? orig_world[ti] : core::Point(), shaded ? orig_normal[ti] : core::Point() },
                        { B, shaded ? orig_world[tj] : core::Point(), shaded ? orig_normal[tj] : core::Point() },
                        { C, shaded ? orig_world[tk] : core::Point(), shaded ? orig_normal[tk] : core::Point() },
                    };
                    const int n = SHClipFixed(poly, 3, wp0, wp1);
                    if (n < 3) continue;

                    for (int i = 0; i < n; ++i) {
                        sink.verts.push_back(poly[i].pos);
                        if (shaded) {
                            sink.world.push_back(poly[i].world);
                            sink.normals.push_back(poly[i].normal);
                        }
                    }
                    // A triangle clipped by the (convex) window is convex, and
                    // Sutherland-Hodgman emits its vertices in order — so a fan is
                    // exactly the right triangulation. The old ear-clipping pass
                    // (O(n^3), plus a by-value polygon copy) was never needed here.
                    for (int i = 1; i + 1 < n; ++i)
                        sink.tris.emplace_back(base, base + (uint32_t)i, base + (uint32_t)i + 1);
                }
            }, buckets);

            std::vector<core::Point> new_world, new_normal;
            if (shaded) {
                // Attribute arrays stay aligned with the final vertex array:
                // [placeholders for line verts] ++ [interpolated tri verts].
                new_world.assign(line_count, core::Point());
                new_normal.assign(line_count, core::Point());
            }
            MergeBuckets(buckets, shaded, line_count,
                         new_verts, new_world, new_normal, new_tris);

            obj.mesh.vertices     = std::move(new_verts);
            obj.mesh.line_indices = std::move(new_lines);
            obj.mesh.tri_indices  = std::move(new_tris);
            if (shaded) {
                obj.mesh.world_vertices = std::move(new_world);
                obj.mesh.world_normals  = std::move(new_normal);
            }

        } else if (obj.type == core::ObjectType::POINT) {
            if (!obj.mesh.vertices.empty()) {
                const auto& v = obj.mesh.vertices[0];
                if (v.x < wp0.x || v.x > wp1.x || v.y < wp0.y || v.y > wp1.y)
                    obj.mesh.vertices.clear();
            }
        } else {
            const std::vector<core::Point> orig_verts = std::move(obj.mesh.vertices);
            const auto orig_lines = std::move(obj.mesh.line_indices);

            std::vector<core::Point> new_verts;
            std::vector<std::pair<uint32_t,uint32_t>> new_lines;
            new_verts.reserve(orig_lines.size() * 2);
            new_lines.reserve(orig_lines.size());

            for (const auto& [i, j] : orig_lines) {
                core::Point a = orig_verts[i];
                core::Point b = orig_verts[j];
                bool survived = (line_clip_mode == 1)
                    ? ClipSegmentCohenSutherland(a, b, wp0, wp1)
                    : ClipSegmentLiangBarsky(a, b, wp0, wp1);
                if (survived) {
                    uint32_t na = (uint32_t)new_verts.size(); new_verts.push_back(a);
                    uint32_t nb = (uint32_t)new_verts.size(); new_verts.push_back(b);
                    new_lines.push_back({na, nb});
                }
            }
            obj.mesh.vertices     = std::move(new_verts);
            obj.mesh.line_indices = std::move(new_lines);
        }
    });
}
