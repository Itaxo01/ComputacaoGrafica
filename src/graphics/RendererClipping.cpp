#include "RendererClipping.hpp"
#include "Triangulate.hpp"
#include "ParallelUtils.hpp"
#include <atomic>
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

// ── Sutherland-Hodgman polygon clipping ───────────────────────────────────────

static bool SHClipping(std::vector<core::Point>& poly,
                        const core::Point& wp0, const core::Point& wp1) {
    for (int edge = 0; edge < 4; ++edge) {
        std::vector<core::Point> input = std::move(poly);
        poly = {};
        if (input.empty()) break;

        core::Point A, B;
        switch (edge) {
            case 0: A = {wp0.x, wp0.y}; B = {wp0.x, wp1.y}; break;
            case 1: A = {wp1.x, wp0.y}; B = {wp1.x, wp1.y}; break;
            case 2: A = {wp0.x, wp0.y}; B = {wp1.x, wp0.y}; break;
            case 3: A = {wp0.x, wp1.y}; B = {wp1.x, wp1.y}; break;
        }

        auto inside = [&](const core::Point& p) -> bool {
            switch (edge) {
                case 0: return p.x >= wp0.x;
                case 1: return p.x <= wp1.x;
                case 2: return p.y >= wp0.y;
                case 3: return p.y <= wp1.y;
            }
            return false;
        };

        for (size_t i = 0; i < input.size(); ++i) {
            const core::Point& cur  = input[i];
            const core::Point& prev = input[(i + input.size() - 1) % input.size()];
            bool ci = inside(cur), pi = inside(prev);
            float denom = (B.x-A.x)*(prev.y-cur.y) - (B.y-A.y)*(prev.x-cur.x);
            if (ci && !pi) {
                if (std::abs(denom) > 1e-9f) {
                    float t = ((A.x-prev.x)*(B.y-A.y) - (A.y-prev.y)*(B.x-A.x)) / denom;
                    poly.push_back({prev.x + t*(cur.x-prev.x), prev.y + t*(cur.y-prev.y)});
                }
                poly.push_back(cur);
            } else if (ci) {
                poly.push_back(cur);
            } else if (pi) {
                if (std::abs(denom) > 1e-9f) {
                    float t = ((A.x-prev.x)*(B.y-A.y) - (A.y-prev.y)*(B.x-A.x)) / denom;
                    poly.push_back({prev.x + t*(cur.x-prev.x), prev.y + t*(cur.y-prev.y)});
                }
            }
        }
    }
    return !poly.empty();
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
    cg_parallel_for_each(objs.begin(), objs.end(), [&](RenderedObject& obj) {
        if (obj.type == core::ObjectType::POINT) {
            if (!obj.mesh.vertices.empty() && obj.mesh.vertices[0].z < near_z)
                obj.mesh.vertices.clear();
            return;
        }

        std::vector<core::Point> new_verts;
        std::vector<std::pair<uint32_t,uint32_t>> new_lines;
        for (auto& [i, j] : obj.mesh.line_indices) {
            core::Point a = obj.mesh.vertices[i];
            core::Point b = obj.mesh.vertices[j];
            if (ClipSegmentNear(a, b, near_z)) {
                uint32_t na = (uint32_t)new_verts.size(); new_verts.push_back(a);
                uint32_t nb = (uint32_t)new_verts.size(); new_verts.push_back(b);
                new_lines.push_back({na, nb});
            }
        }

        // Filled triangles: conservative — keep only those fully in front of the
        // near plane (rare in wireframe scenes; avoids re-triangulating here).
        std::vector<std::tuple<uint32_t,uint32_t,uint32_t>> new_tris;
        for (auto& [ti, tj, tk] : obj.mesh.tri_indices) {
            const core::Point& A = obj.mesh.vertices[ti];
            const core::Point& B = obj.mesh.vertices[tj];
            const core::Point& C = obj.mesh.vertices[tk];
            if (A.z >= near_z && B.z >= near_z && C.z >= near_z) {
                uint32_t base = (uint32_t)new_verts.size();
                new_verts.push_back(A);
                new_verts.push_back(B);
                new_verts.push_back(C);
                new_tris.emplace_back(base, base + 1, base + 2);
            }
        }

        obj.mesh.vertices     = std::move(new_verts);
        obj.mesh.line_indices = std::move(new_lines);
        obj.mesh.tri_indices  = std::move(new_tris);
    });
}

void ClipObjects(std::vector<RenderedObject>& objs,
                 const core::Point& wp0, const core::Point& wp1,
                 int line_clip_mode) {
    cg_parallel_for_each(objs.begin(), objs.end(), [&](auto& obj) {
        if ((obj.type == core::ObjectType::POLYGON || obj.type == core::ObjectType::MESH) && obj.filled) {
            auto orig_verts   = obj.mesh.vertices;
            auto orig_tri_idx = std::move(obj.mesh.tri_indices);

            {
                std::vector<core::Point> new_verts;
                std::vector<std::pair<uint32_t,uint32_t>> new_lines;
                for (auto& [i, j] : obj.mesh.line_indices) {
                    core::Point a = orig_verts[i];
                    core::Point b = orig_verts[j];
                    if (ClipSegmentLiangBarsky(a, b, wp0, wp1)) {
                        uint32_t na = (uint32_t)new_verts.size(); new_verts.push_back(a);
                        uint32_t nb = (uint32_t)new_verts.size(); new_verts.push_back(b);
                        new_lines.push_back({na, nb});
                    }
                }
                obj.mesh.vertices     = new_verts;
                obj.mesh.line_indices = new_lines;
            }

            std::vector<std::tuple<uint32_t,uint32_t,uint32_t>> new_tris;
            std::vector<core::Point> tri_verts;

            for (auto& [ti, tj, tk] : orig_tri_idx) {
                std::vector<core::Point> poly = {orig_verts[ti], orig_verts[tj], orig_verts[tk]};
                if (!SHClipping(poly, wp0, wp1) || poly.size() < 3) continue;

                std::vector<ImVec2> imverts;
                for (const auto& p : poly) imverts.push_back(ImVec2(p.x, p.y));
                auto tris = core::triangulate(imverts);

                uint32_t base = (uint32_t)(obj.mesh.vertices.size() + tri_verts.size());
                for (const auto& p : poly) tri_verts.push_back(p);
                for (int k = 0; k + 2 < (int)tris.size(); k += 3)
                    new_tris.emplace_back(base + tris[k], base + tris[k+1], base + tris[k+2]);
            }

            obj.mesh.vertices.insert(obj.mesh.vertices.end(), tri_verts.begin(), tri_verts.end());
            obj.mesh.tri_indices = std::move(new_tris);

        } else if (obj.type == core::ObjectType::POINT) {
            if (!obj.mesh.vertices.empty()) {
                const auto& v = obj.mesh.vertices[0];
                if (v.x < wp0.x || v.x > wp1.x || v.y < wp0.y || v.y > wp1.y)
                    obj.mesh.vertices.clear();
            }
        } else {
            std::vector<core::Point> new_verts;
            std::vector<std::pair<uint32_t,uint32_t>> new_lines;

            for (auto& [i, j] : obj.mesh.line_indices) {
                core::Point a = obj.mesh.vertices[i];
                core::Point b = obj.mesh.vertices[j];
                bool survived = (line_clip_mode == 1)
                    ? ClipSegmentCohenSutherland(a, b, wp0, wp1)
                    : ClipSegmentLiangBarsky(a, b, wp0, wp1);
                if (survived) {
                    uint32_t na = (uint32_t)new_verts.size(); new_verts.push_back(a);
                    uint32_t nb = (uint32_t)new_verts.size(); new_verts.push_back(b);
                    new_lines.push_back({na, nb});
                }
            }
            obj.mesh.vertices     = new_verts;
            obj.mesh.line_indices = new_lines;
        }
    });
}
