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

static bool SHClipping(std::vector<ClipVert>& poly,
                        const core::Point& wp0, const core::Point& wp1) {
    for (int edge = 0; edge < 4; ++edge) {
        std::vector<ClipVert> input = std::move(poly);
        poly.clear();
        if (input.empty()) break;

        auto inside = [&](const core::Point& p) -> bool {
            switch (edge) {
                case 0: return p.x >= wp0.x;
                case 1: return p.x <= wp1.x;
                case 2: return p.y >= wp0.y;
                case 3: return p.y <= wp1.y;
            }
            return false;
        };
        // Parametric crossing of prev→cur with the (axis-aligned) window edge. The
        // crossing condition (one endpoint inside, one outside) guarantees a nonzero
        // denominator on the relevant axis. Interpolates pos/world/normal together.
        auto intersect = [&](const ClipVert& prev, const ClipVert& cur) -> ClipVert {
            float t;
            switch (edge) {
                case 0: t = (wp0.x - prev.pos.x) / (cur.pos.x - prev.pos.x); break;
                case 1: t = (wp1.x - prev.pos.x) / (cur.pos.x - prev.pos.x); break;
                case 2: t = (wp0.y - prev.pos.y) / (cur.pos.y - prev.pos.y); break;
                default:t = (wp1.y - prev.pos.y) / (cur.pos.y - prev.pos.y); break;
            }
            return lerpCV(prev, cur, t);
        };

        for (size_t i = 0; i < input.size(); ++i) {
            const ClipVert& cur  = input[i];
            const ClipVert& prev = input[(i + input.size() - 1) % input.size()];
            bool ci = inside(cur.pos), pi = inside(prev.pos);
            if (ci && !pi)      { poly.push_back(intersect(prev, cur)); poly.push_back(cur); }
            else if (ci)        { poly.push_back(cur); }
            else if (pi)        { poly.push_back(intersect(prev, cur)); }
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

        const bool shaded = !obj.mesh.world_vertices.empty();
        std::vector<core::Point> new_verts, new_world, new_normal;
        std::vector<std::pair<uint32_t,uint32_t>> new_lines;
        for (auto& [i, j] : obj.mesh.line_indices) {
            core::Point a = obj.mesh.vertices[i];
            core::Point b = obj.mesh.vertices[j];
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
                if (shaded) {
                    new_world.push_back(obj.mesh.world_vertices[ti]);
                    new_world.push_back(obj.mesh.world_vertices[tj]);
                    new_world.push_back(obj.mesh.world_vertices[tk]);
                    new_normal.push_back(obj.mesh.world_normals[ti]);
                    new_normal.push_back(obj.mesh.world_normals[tj]);
                    new_normal.push_back(obj.mesh.world_normals[tk]);
                }
            }
        }

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
    cg_parallel_for_each(objs.begin(), objs.end(), [&](auto& obj) {
        if ((obj.type == core::ObjectType::POLYGON || obj.type == core::ObjectType::MESH) && obj.filled) {
            auto orig_verts   = obj.mesh.vertices;
            auto orig_tri_idx = std::move(obj.mesh.tri_indices);
            // Shading attributes ride through the clip (interpolated by SHClipping).
            const bool shaded = !obj.mesh.world_vertices.empty();
            auto orig_world   = obj.mesh.world_vertices;
            auto orig_normal  = obj.mesh.world_normals;

            // Wireframe edges (Liang-Barsky). These verts come first in the array;
            // shading only reads tri-vertex indices, so they get attribute placeholders.
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
            const size_t line_count = obj.mesh.vertices.size();

            std::vector<std::tuple<uint32_t,uint32_t,uint32_t>> new_tris;
            std::vector<core::Point> tri_verts, tri_world, tri_normal;

            for (auto& [ti, tj, tk] : orig_tri_idx) {
                std::vector<ClipVert> poly = {
                    { orig_verts[ti], shaded ? orig_world[ti] : core::Point(), shaded ? orig_normal[ti] : core::Point() },
                    { orig_verts[tj], shaded ? orig_world[tj] : core::Point(), shaded ? orig_normal[tj] : core::Point() },
                    { orig_verts[tk], shaded ? orig_world[tk] : core::Point(), shaded ? orig_normal[tk] : core::Point() },
                };
                if (!SHClipping(poly, wp0, wp1) || poly.size() < 3) continue;

                std::vector<ImVec2> imverts;
                for (const auto& p : poly) imverts.push_back(ImVec2(p.pos.x, p.pos.y));
                auto tris = core::triangulate(imverts);

                uint32_t base = (uint32_t)(line_count + tri_verts.size());
                for (const auto& p : poly) {
                    tri_verts.push_back(p.pos);
                    if (shaded) { tri_world.push_back(p.world); tri_normal.push_back(p.normal); }
                }
                for (int k = 0; k + 2 < (int)tris.size(); k += 3)
                    new_tris.emplace_back(base + tris[k], base + tris[k+1], base + tris[k+2]);
            }

            obj.mesh.vertices.insert(obj.mesh.vertices.end(), tri_verts.begin(), tri_verts.end());
            obj.mesh.tri_indices = std::move(new_tris);

            if (shaded) {
                // Rebuild attribute arrays aligned with the final vertex array:
                // [placeholders for line verts] ++ [interpolated tri verts].
                obj.mesh.world_vertices.assign(line_count, core::Point());
                obj.mesh.world_normals.assign(line_count, core::Point());
                obj.mesh.world_vertices.insert(obj.mesh.world_vertices.end(), tri_world.begin(), tri_world.end());
                obj.mesh.world_normals.insert(obj.mesh.world_normals.end(), tri_normal.begin(), tri_normal.end());
            }

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
