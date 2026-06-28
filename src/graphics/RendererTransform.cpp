#include "RendererTransform.hpp"
#include "ParallelUtils.hpp"
#include "AppConfig.hpp"
#include "Lighting.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <iterator>

// As funções de transformação paralelizam sobre o POOL GLOBAL de vértices da cena
// (não mais sobre objetos): um único loop plano sobre [0, vertexCount). Para um
// mesh pesado isso já distribui o trabalho entre os cores, e é exatamente a forma
// que um kernel CUDA assume (um thread por vértice). O limiar evita o overhead de
// paralelizar trabalhos pequenos.
namespace {
    constexpr std::size_t kVertexParallelThreshold   = 16384;
    constexpr std::size_t kTriangleParallelThreshold = 16384;

    // Executa body(i) para i em [0, n): em paralelo se n for grande, senão serial.
    template <typename F>
    inline void for_range(std::size_t n, F&& body) {
        if (n >= kVertexParallelThreshold) {
            cg_parallel_chunks(n, [&](std::size_t lo, std::size_t hi) {
                for (std::size_t i = lo; i < hi; ++i) body(i);
            });
        } else {
            for (std::size_t i = 0; i < n; ++i) body(i);
        }
    }
}

void BuildGeometryBuffer(const std::vector<core::Object>& src,
                         GeometryBuffer& gb, std::vector<ObjectSlice>& slices, bool shading) {
    gb.clear();
    const size_t nObj = src.size();
    slices.assign(nObj, ObjectSlice{});

    // First pass (sequential, #objects): per-object offsets into the flat arrays and
    // the ObjectSlice table. Cheap relative to the vertex copy below.
    std::vector<size_t> vBase(nObj), pBase(nObj), lBase(nObj), tBase(nObj);
    size_t totV = 0, totP = 0, totL = 0, totT = 0;
    for (size_t o = 0; o < nObj; ++o) {
        const core::Object& obj = src[o];
        vBase[o] = totV; pBase[o] = totP; lBase[o] = totL; tBase[o] = totT;
        const size_t nv = obj.mesh->vertices.size();
        totV += nv;
        if (obj.type == core::ObjectType::POINT) { if (nv > 0) totP += 1; }
        else { totL += obj.mesh->line_indices.size(); totT += obj.mesh->tri_indices.size(); }

        ObjectSlice s;
        s.type = obj.type; s.color = obj.material.color; s.filled = obj.material.filled;
        s.transform = obj.transform;
        // Material subset. Many .mtl set Ka=0; fall back to Kd so ambient isn't black.
        core::Color3 ka = obj.material.ambient;
        if (ka.r == 0.0f && ka.g == 0.0f && ka.b == 0.0f) ka = obj.material.diffuse;
        s.shadeMat = { ka, obj.material.diffuse, obj.material.specular, obj.material.shininess };
        slices[o] = s;
    }

    gb.pos.assign(3 * totV, 0.0f);
    gb.vobj.assign(totV, 0);
    if (shading) { gb.world.assign(3 * totV, 0.0f); gb.normal.assign(3 * totV, 0.0f); }
    gb.pointIdx.assign(totP, 0); gb.pointObj.assign(totP, 0);
    gb.lineIdx.assign(2 * totL, 0); gb.lineObj.assign(totL, 0);
    gb.triIdx.assign(3 * totT, 0); gb.triObj.assign(totT, 0);

    // Second pass: copy model positions + remap indices into the global pool.
    // Parallel over objects (one big mesh = one chunk = a flat memcpy-ish loop).
    cg_parallel_chunks(nObj, [&](size_t lo, size_t hi) {
        for (size_t o = lo; o < hi; ++o) {
            const core::Object& obj = src[o];
            const size_t vb = vBase[o];
            const auto& verts = obj.mesh->vertices;
            for (size_t j = 0; j < verts.size(); ++j) {
                gb.pos[3 * (vb + j) + 0] = verts[j].x;
                gb.pos[3 * (vb + j) + 1] = verts[j].y;
                gb.pos[3 * (vb + j) + 2] = verts[j].z;
                gb.vobj[vb + j] = (int32_t)o;
            }
            if (obj.type == core::ObjectType::POINT) {
                if (!verts.empty()) {
                    gb.pointIdx[pBase[o]] = (uint32_t)vb;
                    gb.pointObj[pBase[o]] = (int32_t)o;
                }
                continue;
            }
            const auto& li = obj.mesh->line_indices;
            for (size_t k = 0; k < li.size(); ++k) {
                gb.lineIdx[2 * (lBase[o] + k) + 0] = (uint32_t)(vb + li[k].first);
                gb.lineIdx[2 * (lBase[o] + k) + 1] = (uint32_t)(vb + li[k].second);
                gb.lineObj[lBase[o] + k] = (int32_t)o;
            }
            const auto& ti = obj.mesh->tri_indices;
            for (size_t k = 0; k < ti.size(); ++k) {
                const auto& [a, b, c] = ti[k];
                gb.triIdx[3 * (tBase[o] + k) + 0] = (uint32_t)(vb + a);
                gb.triIdx[3 * (tBase[o] + k) + 1] = (uint32_t)(vb + b);
                gb.triIdx[3 * (tBase[o] + k) + 2] = (uint32_t)(vb + c);
                gb.triObj[tBase[o] + k] = (int32_t)o;
            }
        }
    });
}

void TransformFlat(GeometryBuffer& gb, const std::vector<ObjectSlice>& slices,
                   const core::mat4& ncs_mat, bool shading) {
    const size_t nv = gb.vertexCount();

    if (!shading) {
        gb.world.clear(); gb.normal.clear();
        // Pre-combine ncs * transform per object so the per-vertex loop is a single
        // matrix multiply (the matrices are tiny and there are few objects).
        std::vector<core::mat4> comb(slices.size());
        for (size_t o = 0; o < slices.size(); ++o) comb[o] = ncs_mat * slices[o].transform;
        for_range(nv, [&](size_t i) {
            const core::mat4& m = comb[gb.vobj[i]];
            core::Point r = m * core::Point(gb.pos[3*i], gb.pos[3*i+1], gb.pos[3*i+2]);
            gb.pos[3*i] = r.x; gb.pos[3*i+1] = r.y; gb.pos[3*i+2] = r.z;
        });
        return;
    }

    // Shading on: world = transform * v (kept for lighting); pos = ncs * world.
    if (gb.world.size()  != 3*nv) gb.world.assign(3*nv, 0.0f);
    if (gb.normal.size() != 3*nv) gb.normal.assign(3*nv, 0.0f);
    for_range(nv, [&](size_t i) {
        const core::mat4& t = slices[gb.vobj[i]].transform;
        core::Point wv = t * core::Point(gb.pos[3*i], gb.pos[3*i+1], gb.pos[3*i+2]);
        gb.world[3*i] = wv.x; gb.world[3*i+1] = wv.y; gb.world[3*i+2] = wv.z;
        core::Point r = ncs_mat * wv;
        gb.pos[3*i] = r.x; gb.pos[3*i+1] = r.y; gb.pos[3*i+2] = r.z;
    });

    // Smooth world-space normals: accumulate (area-weighted) world face normals onto
    // shared vertices, then normalize. The scatter is run serially because triangles
    // of the same object share vertices (write race); on the GPU this is the standard
    // atomic-add accumulation. Normalization is a plain per-vertex map.
    std::fill(gb.normal.begin(), gb.normal.end(), 0.0f);
    const size_t nt = gb.triCount();
    for (size_t t = 0; t < nt; ++t) {
        uint32_t ia = gb.triIdx[3*t], ib = gb.triIdx[3*t+1], ic = gb.triIdx[3*t+2];
        core::Point A = gb.getWorld(ia), B = gb.getWorld(ib), C = gb.getWorld(ic);
        core::Point fn = cross(B - A, C - A);
        gb.normal[3*ia]+=fn.x; gb.normal[3*ia+1]+=fn.y; gb.normal[3*ia+2]+=fn.z;
        gb.normal[3*ib]+=fn.x; gb.normal[3*ib+1]+=fn.y; gb.normal[3*ib+2]+=fn.z;
        gb.normal[3*ic]+=fn.x; gb.normal[3*ic+1]+=fn.y; gb.normal[3*ic+2]+=fn.z;
    }
    for_range(nv, [&](size_t i) {
        core::Point n = gb.getNormal((uint32_t)i);
        float l = std::sqrt(dot(n, n));
        if (l > 1e-12f) { gb.normal[3*i]=n.x/l; gb.normal[3*i+1]=n.y/l; gb.normal[3*i+2]=n.z/l; }
    });
}

void ProjectFlat(GeometryBuffer& gb, const core::mat4& mat) {
    const size_t nv = gb.vertexCount();
    for_range(nv, [&](size_t i) {
        core::Point r = mat * core::Point(gb.pos[3*i], gb.pos[3*i+1], gb.pos[3*i+2]);
        gb.pos[3*i] = r.x; gb.pos[3*i+1] = r.y; gb.pos[3*i+2] = r.z;
    });
}

void ViewportFlat(GeometryBuffer& gb, const Window& window, float scale) {
    // Expanded layout invariant: point verts, then line verts, then tri verts. Tri
    // verts were already consumed (in NCS) by BuildSortedTrianglesFlat, so only the
    // point/line verts still need mapping to pixel space.
    const size_t lineVertEnd = gb.pointCount() + 2 * gb.lineCount();
    for_range(lineVertEnd, [&](size_t i) {
        core::Point s = window.NCSToViewport(core::Point(gb.pos[3*i], gb.pos[3*i+1], gb.pos[3*i+2]));
        gb.pos[3*i] = s.x * scale;
        gb.pos[3*i+1] = s.y * scale;
        // z keeps the NCS depth so wireframe lines/points can be depth-tested.
    });
}

void BuildSortedTrianglesFlat(const GeometryBuffer& gb, const std::vector<ObjectSlice>& slices,
                              const Window& window, float scale, std::vector<SortedTri>& out) {
    out.clear();
    const bool cull = AppConfig::is3d && AppConfig::backface_cull;
    const bool shaded = gb.shaded();
    const size_t nt = gb.triCount();
    out.reserve(nt);

    std::mutex out_mtx;

    auto build_range = [&](size_t lo, size_t hi, std::vector<SortedTri>& sink) {
        for (size_t t = lo; t < hi; ++t) {
            const ObjectSlice& sl = slices[gb.triObj[t]];
            // Only cull imported meshes; user-drawn filled polygons are single-sided
            // surfaces that must stay visible regardless of winding.
            const bool cullThis = cull && (sl.type == core::ObjectType::MESH);

            uint32_t ia = gb.triIdx[3*t], ib = gb.triIdx[3*t+1], ic = gb.triIdx[3*t+2];
            core::Point A = gb.getPos(ia), B = gb.getPos(ib), C = gb.getPos(ic); // NCS: z is depth

            core::Point va = window.NCSToViewport(A); // maps x/y to screen, drops z
            core::Point vb = window.NCSToViewport(B);
            core::Point vc = window.NCSToViewport(C);

            float area = (vb.x - va.x) * (vc.y - va.y) - (vb.y - va.y) * (vc.x - va.x);
            if (cullThis) {
                bool back = AppConfig::cull_ccw ? (area <= 0.0f) : (area >= 0.0f);
                if (back) continue;
            }

            SortedTri tt;
            tt.a = ImVec2(va.x * scale, va.y * scale);
            tt.b = ImVec2(vb.x * scale, vb.y * scale);
            tt.c = ImVec2(vc.x * scale, vc.y * scale);
            tt.color = sl.color;
            tt.depth = (A.z + B.z + C.z) / 3.0f;
            tt.za = A.z; tt.zb = B.z; tt.zc = C.z;
            if (shaded) {
                tt.P[0] = gb.getWorld(ia);  tt.P[1] = gb.getWorld(ib);  tt.P[2] = gb.getWorld(ic);
                tt.N[0] = gb.getNormal(ia); tt.N[1] = gb.getNormal(ib); tt.N[2] = gb.getNormal(ic);
                tt.mat = sl.shadeMat;
            }
            sink.push_back(tt);
        }
    };

    if (nt >= kTriangleParallelThreshold) {
        cg_parallel_chunks(nt, [&](size_t lo, size_t hi) {
            std::vector<SortedTri> local;
            local.reserve(hi - lo);
            build_range(lo, hi, local);
            std::lock_guard<std::mutex> g(out_mtx);
            out.insert(out.end(),
                       std::make_move_iterator(local.begin()),
                       std::make_move_iterator(local.end()));
        });
    } else {
        build_range(0, nt, out);
    }

    // The painter's sort is pointless when the z-buffer resolves visibility per pixel.
    if (AppConfig::is3d && AppConfig::depth_sort && !AppConfig::z_buffer) {
        std::sort(out.begin(), out.end(), [](const SortedTri& a, const SortedTri& b) {
            return AppConfig::depth_ascending ? (a.depth < b.depth) : (a.depth > b.depth);
        });
    }
}
