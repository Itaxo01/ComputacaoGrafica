#include "RendererClipping.hpp"
#include "Triangulate.hpp"
#include "ParallelUtils.hpp"
#include <vector>
#include <cstdint>
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
// aligned with the geometry after clipping). When shading is off, world/normal are
// just unused zeros riding along.
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

// ── Flat clip plumbing: bounded per-primitive output + prefix-sum compaction ──

// Exclusive prefix sum over per-primitive output counts. Returns the total; fills
// `offsets[i]` with where primitive i's outputs start in the packed result. On the
// GPU this is a parallel scan; here a serial scan suffices for the CPU proof.
static size_t exclusiveScan(const std::vector<uint32_t>& counts, std::vector<uint32_t>& offsets) {
    offsets.resize(counts.size());
    size_t acc = 0;
    for (size_t i = 0; i < counts.size(); ++i) { offsets[i] = (uint32_t)acc; acc += counts[i]; }
    return acc;
}

// Generic clip-stage assembler. Each stage supplies three primitive "clippers" that
// emit their (0..K) outputs through a callback; the assembler runs them twice — once
// to count, once (after the scan) to write into the tightly-packed EXPANDED output
// buffer. Re-running the clip in the write pass keeps memory minimal (only the small
// count/offset arrays are stored), which is the canonical scan/scatter shape used on
// the GPU. The output layout is [point verts][line verts][tri verts].
//
//   pointClip(i, emit)  emit(core::Point p)
//   lineClip(i, emit)   emit(core::Point a, core::Point b)
//   triClip(i, emit)    emit(ClipVert a, ClipVert b, ClipVert c)
template <class PointFn, class LineFn, class TriFn>
static GeometryBuffer assembleClipped(const GeometryBuffer& in, bool shaded,
                                      PointFn pointClip, LineFn lineClip, TriFn triClip) {
    const size_t nP = in.pointCount(), nL = in.lineCount(), nT = in.triCount();
    std::vector<uint32_t> pc(nP, 0), lc(nL, 0), tc(nT, 0);

    // ── Count pass ──
    cg_parallel_chunks(nP, [&](size_t lo, size_t hi) {
        for (size_t i = lo; i < hi; ++i) { uint32_t c = 0; pointClip(i, [&](const core::Point&) { ++c; }); pc[i] = c; }
    });
    cg_parallel_chunks(nL, [&](size_t lo, size_t hi) {
        for (size_t i = lo; i < hi; ++i) { uint32_t c = 0; lineClip(i, [&](const core::Point&, const core::Point&) { ++c; }); lc[i] = c; }
    });
    cg_parallel_chunks(nT, [&](size_t lo, size_t hi) {
        for (size_t i = lo; i < hi; ++i) { uint32_t c = 0; triClip(i, [&](const ClipVert&, const ClipVert&, const ClipVert&) { ++c; }); tc[i] = c; }
    });

    std::vector<uint32_t> po, lo2, to2;
    const size_t P = exclusiveScan(pc, po);
    const size_t L = exclusiveScan(lc, lo2);
    const size_t T = exclusiveScan(tc, to2);

    GeometryBuffer out;
    const size_t V = P + 2 * L + 3 * T;
    out.pos.assign(3 * V, 0.0f);
    if (shaded) { out.world.assign(3 * V, 0.0f); out.normal.assign(3 * V, 0.0f); }
    out.pointIdx.assign(P, 0); out.pointObj.assign(P, 0);
    out.lineIdx.assign(2 * L, 0); out.lineObj.assign(L, 0);
    out.triIdx.assign(3 * T, 0); out.triObj.assign(T, 0);
    const size_t pvBase = 0, lvBase = P, tvBase = P + 2 * L;

    // ── Write pass ── (each primitive owns a disjoint output region → race-free)
    cg_parallel_chunks(nP, [&](size_t lo, size_t hi) {
        for (size_t i = lo; i < hi; ++i) {
            if (pc[i] == 0) continue;
            const uint32_t o = po[i];
            const uint32_t v = (uint32_t)(pvBase + o);
            pointClip(i, [&](const core::Point& p) {
                out.pos[3*v] = p.x; out.pos[3*v+1] = p.y; out.pos[3*v+2] = p.z;
            });
            out.pointIdx[o] = v; out.pointObj[o] = in.pointObj[i];
        }
    });
    cg_parallel_chunks(nL, [&](size_t lo, size_t hi) {
        for (size_t i = lo; i < hi; ++i) {
            if (lc[i] == 0) continue;
            const uint32_t o = lo2[i];
            const uint32_t vb = (uint32_t)(lvBase + 2 * o);
            lineClip(i, [&](const core::Point& a, const core::Point& b) {
                out.pos[3*vb] = a.x;     out.pos[3*vb+1] = a.y;     out.pos[3*vb+2] = a.z;
                out.pos[3*(vb+1)] = b.x; out.pos[3*(vb+1)+1] = b.y; out.pos[3*(vb+1)+2] = b.z;
            });
            out.lineIdx[2*o] = vb; out.lineIdx[2*o+1] = vb + 1; out.lineObj[o] = in.lineObj[i];
        }
    });
    cg_parallel_chunks(nT, [&](size_t lo, size_t hi) {
        for (size_t i = lo; i < hi; ++i) {
            if (tc[i] == 0) continue;
            const uint32_t baseTri = to2[i];
            uint32_t k = 0;
            triClip(i, [&](const ClipVert& a, const ClipVert& b, const ClipVert& c) {
                const uint32_t g = baseTri + k;
                const uint32_t vb = (uint32_t)(tvBase + 3 * g);
                const ClipVert* cv[3] = { &a, &b, &c };
                for (int e = 0; e < 3; ++e) {
                    out.pos[3*(vb+e)]   = cv[e]->pos.x; out.pos[3*(vb+e)+1] = cv[e]->pos.y; out.pos[3*(vb+e)+2] = cv[e]->pos.z;
                    if (shaded) {
                        out.world[3*(vb+e)]  = cv[e]->world.x;  out.world[3*(vb+e)+1]  = cv[e]->world.y;  out.world[3*(vb+e)+2]  = cv[e]->world.z;
                        out.normal[3*(vb+e)] = cv[e]->normal.x; out.normal[3*(vb+e)+1] = cv[e]->normal.y; out.normal[3*(vb+e)+2] = cv[e]->normal.z;
                    }
                    out.triIdx[3*g+e] = vb + e;
                }
                out.triObj[g] = in.triObj[i];
                ++k;
            });
        }
    });

    return out;
}

GeometryBuffer NearClipFlat(const GeometryBuffer& in, float near_z) {
    const bool shaded = in.shaded();

    auto pointClip = [&](size_t i, auto emit) {
        core::Point p = in.getPos(in.pointIdx[i]);
        if (p.z >= near_z) emit(p);
    };
    auto lineClip = [&](size_t i, auto emit) {
        core::Point a = in.getPos(in.lineIdx[2*i]);
        core::Point b = in.getPos(in.lineIdx[2*i+1]);
        if (ClipSegmentNear(a, b, near_z)) emit(a, b);
    };
    // Filled triangles: conservative — keep only those fully in front of the near
    // plane (kept whole, so attributes are copied, no interpolation needed).
    auto triClip = [&](size_t i, auto emit) {
        uint32_t ia = in.triIdx[3*i], ib = in.triIdx[3*i+1], ic = in.triIdx[3*i+2];
        core::Point A = in.getPos(ia), B = in.getPos(ib), C = in.getPos(ic);
        if (A.z >= near_z && B.z >= near_z && C.z >= near_z) {
            emit(ClipVert{ A, shaded ? in.getWorld(ia) : core::Point(), shaded ? in.getNormal(ia) : core::Point() },
                 ClipVert{ B, shaded ? in.getWorld(ib) : core::Point(), shaded ? in.getNormal(ib) : core::Point() },
                 ClipVert{ C, shaded ? in.getWorld(ic) : core::Point(), shaded ? in.getNormal(ic) : core::Point() });
        }
    };

    return assembleClipped(in, shaded, pointClip, lineClip, triClip);
}

GeometryBuffer BoxClipFlat(const GeometryBuffer& in, const std::vector<ObjectSlice>& slices,
                           const core::Point& wp0, const core::Point& wp1, int line_clip_mode) {
    const bool shaded = in.shaded();
    auto isFilled = [&](int o) {
        const ObjectSlice& s = slices[o];
        return (s.type == core::ObjectType::POLYGON || s.type == core::ObjectType::MESH) && s.filled;
    };

    auto pointClip = [&](size_t i, auto emit) {
        core::Point p = in.getPos(in.pointIdx[i]);
        if (p.x >= wp0.x && p.x <= wp1.x && p.y >= wp0.y && p.y <= wp1.y) emit(p);
    };
    auto lineClip = [&](size_t i, auto emit) {
        core::Point a = in.getPos(in.lineIdx[2*i]);
        core::Point b = in.getPos(in.lineIdx[2*i+1]);
        // Filled polygon/mesh outlines always use Liang-Barsky; other (wireframe)
        // objects honor the user's chosen line clip mode.
        bool survived = isFilled(in.lineObj[i])
            ? ClipSegmentLiangBarsky(a, b, wp0, wp1)
            : (line_clip_mode == 1 ? ClipSegmentCohenSutherland(a, b, wp0, wp1)
                                   : ClipSegmentLiangBarsky(a, b, wp0, wp1));
        if (survived) emit(a, b);
    };
    auto triClip = [&](size_t i, auto emit) {
        if (!isFilled(in.triObj[i])) return; // only filled surfaces contribute fills
        uint32_t ia = in.triIdx[3*i], ib = in.triIdx[3*i+1], ic = in.triIdx[3*i+2];
        std::vector<ClipVert> poly = {
            { in.getPos(ia), shaded ? in.getWorld(ia) : core::Point(), shaded ? in.getNormal(ia) : core::Point() },
            { in.getPos(ib), shaded ? in.getWorld(ib) : core::Point(), shaded ? in.getNormal(ib) : core::Point() },
            { in.getPos(ic), shaded ? in.getWorld(ic) : core::Point(), shaded ? in.getNormal(ic) : core::Point() },
        };
        if (!SHClipping(poly, wp0, wp1) || poly.size() < 3) return;

        std::vector<ImVec2> imverts;
        imverts.reserve(poly.size());
        for (const auto& p : poly) imverts.push_back(ImVec2(p.pos.x, p.pos.y));
        std::vector<int> tris = core::triangulate(imverts);
        for (int k = 0; k + 2 < (int)tris.size(); k += 3)
            emit(poly[tris[k]], poly[tris[k+1]], poly[tris[k+2]]);
    };

    return assembleClipped(in, shaded, pointClip, lineClip, triClip);
}
