#include "RendererTransform.hpp"
#include "ParallelUtils.hpp"
#include "AppConfig.hpp"
#include "Lighting.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <iterator>

// As funções de transformação paralelizam sobre o vetor de OBJETOS. Numa cena com
// um único mesh pesado (ex.: o donut girando), isso degenera para 1 thread. Para
// esses casos, paralelizamos também DENTRO do mesh, sobre os vértices/triângulos.
// O limiar evita sobre-subscrição quando há muitos objetos pequenos (aí o
// paralelismo por-objeto já satura os cores e o trabalho por-mesh é leve).
namespace {
    constexpr std::size_t kVertexParallelThreshold   = 16384;
    constexpr std::size_t kTriangleParallelThreshold = 16384;

    // Executa body(j) para j em [0, n): em paralelo se n for grande, senão serial.
    template <typename F>
    inline void for_vertices(std::size_t n, F&& body) {
        if (n >= kVertexParallelThreshold) {
            cg_parallel_chunks(n, [&](std::size_t lo, std::size_t hi) {
                for (std::size_t j = lo; j < hi; ++j) body(j);
            });
        } else {
            for (std::size_t j = 0; j < n; ++j) body(j);
        }
    }
}

void TransformObjectAndDoNCS(std::vector<RenderedObject>& dest,
                              const std::vector<core::Object>& src,
                              const core::mat4& ncs_mat) {
    dest.resize(src.size());
    const bool shading = Lighting::mode != Lighting::NONE;
    cg_parallel_for_each(dest.begin(), dest.end(), [&](RenderedObject& ro) {
        size_t i = &ro - dest.data();
        const core::Object& obj = src[i];
        ro.type     = obj.type;
        ro.color    = obj.material.color;
        ro.filled   = obj.material.filled;
        ro.mesh.line_indices = obj.mesh->line_indices;
        ro.mesh.tri_indices  = obj.mesh->tri_indices;
        const size_t nv = obj.mesh->vertices.size();
        ro.mesh.vertices.resize(nv);

        if (!shading) {
            core::mat4 combined = ncs_mat * obj.transform;
            for_vertices(nv, [&](size_t j) {
                ro.mesh.vertices[j] = combined * obj.mesh->vertices[j];
            });
            ro.mesh.world_vertices.clear();
            ro.mesh.world_normals.clear();
            return;
        }

        // Shading on: keep world-space positions (for lighting) separately from the
        // NCS/VRC working positions. world = obj.transform * v; working = ncs * world.
        ro.mesh.world_vertices.resize(nv);
        for_vertices(nv, [&](size_t j) {
            core::Point wv = obj.transform * obj.mesh->vertices[j];
            ro.mesh.world_vertices[j] = wv;
            ro.mesh.vertices[j] = ncs_mat * wv;
        });

        // Smooth world-space vertex normals: sum the (area-weighted) world face
        // normals over shared vertices, then normalize. Cross of world-space edges
        // is already the world normal, so no inverse-transpose matrix is needed.
        auto& wn = ro.mesh.world_normals;
        wn.assign(nv, core::Point(0.0f, 0.0f, 0.0f));
        for (const auto& [ti, tj, tk] : ro.mesh.tri_indices) {
            const core::Point& A = ro.mesh.world_vertices[ti];
            const core::Point& B = ro.mesh.world_vertices[tj];
            const core::Point& C = ro.mesh.world_vertices[tk];
            core::Point fn = cross(B - A, C - A);
            wn[ti] += fn; wn[tj] += fn; wn[tk] += fn;
        }
        for (auto& n : wn) {
            float l = std::sqrt(dot(n, n));
            if (l > 1e-12f) n /= l;
        }

        // Material subset. Many .mtl set Ka=0; fall back to Kd so ambient isn't black.
        core::Color3 ka = obj.material.ambient;
        if (ka.r == 0.0f && ka.g == 0.0f && ka.b == 0.0f) ka = obj.material.diffuse;
        ro.shadeMat = { ka, obj.material.diffuse, obj.material.specular, obj.material.shininess };
    });
}

void ProjectVertices(std::vector<RenderedObject>& objs, const core::mat4& mat) {
    cg_parallel_for_each(objs.begin(), objs.end(), [&](RenderedObject& obj) {
        auto& verts = obj.mesh.vertices;
        for_vertices(verts.size(), [&](size_t j) { verts[j] = mat * verts[j]; });
    });
}

void TransformToViewport(std::vector<RenderedObject>& objs, const Window& window, float scale) {
    cg_parallel_for_each(objs.begin(), objs.end(), [&](RenderedObject& obj) {
        auto& verts = obj.mesh.vertices;
        for_vertices(verts.size(), [&](size_t j) {
            core::Point s = window.NCSToViewport(verts[j]); // maps x/y to viewport, drops z
            verts[j].x = s.x * scale;
            verts[j].y = s.y * scale;
            // z keeps the NCS depth so wireframe lines/points can be depth-tested.
        });
    });
}

void BuildSortedTriangles(const std::vector<RenderedObject>& objs, const Window& window,
                          float scale, std::vector<SortedTri>& out) {
    out.clear();
    const bool cull = AppConfig::is3d && AppConfig::backface_cull;

    // Reserve the upper bound (pre-cull total) once so the parallel splices below
    // never reallocate mid-flight — fewer allocations and less memory churn.
    size_t total_tris = 0;
    for (const auto& o : objs) total_tris += o.mesh.tri_indices.size();
    out.reserve(total_tris);

    std::mutex out_mtx;

    for (const auto& o : objs) {
        if (o.mesh.tri_indices.empty()) continue;
        // Only cull imported meshes; user-drawn filled polygons are single-sided
        // surfaces that must stay visible regardless of winding.
        const bool cullThis = cull && (o.type == core::ObjectType::MESH);
        const bool shaded = !o.mesh.world_vertices.empty();
        const size_t nt = o.mesh.tri_indices.size();

        // Builds the screen triangles for indices [lo, hi) of this object into sink.
        auto build_range = [&](size_t lo, size_t hi, std::vector<SortedTri>& sink) {
            for (size_t idx = lo; idx < hi; ++idx) {
                const auto& [ti, tj, tk] = o.mesh.tri_indices[idx];
                const core::Point& A = o.mesh.vertices[ti];   // NCS space: z is depth
                const core::Point& B = o.mesh.vertices[tj];
                const core::Point& C = o.mesh.vertices[tk];

                core::Point va = window.NCSToViewport(A);  // maps x/y to screen, drops z
                core::Point vb = window.NCSToViewport(B);
                core::Point vc = window.NCSToViewport(C);

                float area = (vb.x - va.x) * (vc.y - va.y) - (vb.y - va.y) * (vc.x - va.x);
                if (cullThis) {
                    bool back = AppConfig::cull_ccw ? (area <= 0.0f) : (area >= 0.0f);
                    if (back) continue;
                }

                SortedTri t;
                t.a = ImVec2(va.x * scale, va.y * scale);
                t.b = ImVec2(vb.x * scale, vb.y * scale);
                t.c = ImVec2(vc.x * scale, vc.y * scale);
                t.color = o.color;
                t.depth = (A.z + B.z + C.z) / 3.0f;
                t.za = A.z; t.zb = B.z; t.zc = C.z;
                if (shaded) {
                    t.P[0] = o.mesh.world_vertices[ti];
                    t.P[1] = o.mesh.world_vertices[tj];
                    t.P[2] = o.mesh.world_vertices[tk];
                    t.N[0] = o.mesh.world_normals[ti];
                    t.N[1] = o.mesh.world_normals[tj];
                    t.N[2] = o.mesh.world_normals[tk];
                    t.mat = o.shadeMat;
                }
                sink.push_back(t);
            }
        };

        // Big mesh (the donut case): split the per-triangle work across threads into
        // thread-local buckets, then splice under a coarse lock (~one lock/thread).
        // Output order is irrelevant: the painter's sort below reorders anyway, and
        // with the z-buffer visibility is resolved per pixel.
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
    }

    // The painter's sort is pointless when the z-buffer resolves visibility per pixel.
    if (AppConfig::is3d && AppConfig::depth_sort && !AppConfig::z_buffer) {
        std::sort(out.begin(), out.end(), [](const SortedTri& a, const SortedTri& b) {
            return AppConfig::depth_ascending ? (a.depth < b.depth) : (a.depth > b.depth);
        });
    }
}
