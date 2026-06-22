#include "RendererTransform.hpp"
#include "ParallelUtils.hpp"
#include "AppConfig.hpp"
#include "Lighting.hpp"
#include <algorithm>
#include <cmath>

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
            for (size_t j = 0; j < nv; ++j)
                ro.mesh.vertices[j] = combined * obj.mesh->vertices[j];
            ro.mesh.world_vertices.clear();
            ro.mesh.world_normals.clear();
            return;
        }

        // Shading on: keep world-space positions (for lighting) separately from the
        // NCS/VRC working positions. world = obj.transform * v; working = ncs * world.
        ro.mesh.world_vertices.resize(nv);
        for (size_t j = 0; j < nv; ++j) {
            core::Point wv = obj.transform * obj.mesh->vertices[j];
            ro.mesh.world_vertices[j] = wv;
            ro.mesh.vertices[j] = ncs_mat * wv;
        }

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
        for (auto& v : obj.mesh.vertices)
            v = mat * v;
    });
}

void TransformToViewport(std::vector<RenderedObject>& objs, const Window& window, float scale) {
    cg_parallel_for_each(objs.begin(), objs.end(), [&](RenderedObject& obj) {
        for (auto& v : obj.mesh.vertices) {
            core::Point s = window.NCSToViewport(v); // maps x/y to viewport, drops z
            v.x = s.x * scale;
            v.y = s.y * scale;
            // v.z keeps the NCS depth so wireframe lines/points can be depth-tested.
        }
    });
}

void BuildSortedTriangles(const std::vector<RenderedObject>& objs, const Window& window,
                          float scale, std::vector<SortedTri>& out) {
    out.clear();
    const bool cull = AppConfig::is3d && AppConfig::backface_cull;

    for (const auto& o : objs) {
        if (o.mesh.tri_indices.empty()) continue;
        // Only cull imported meshes; user-drawn filled polygons are single-sided
        // surfaces that must stay visible regardless of winding.
        const bool cullThis = cull && (o.type == core::ObjectType::MESH);
        const bool shaded = !o.mesh.world_vertices.empty();
        for (const auto& [ti, tj, tk] : o.mesh.tri_indices) {
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
            out.push_back(t);
        }
    }

    // The painter's sort is pointless when the z-buffer resolves visibility per pixel.
    if (AppConfig::is3d && AppConfig::depth_sort && !AppConfig::z_buffer) {
        std::sort(out.begin(), out.end(), [](const SortedTri& a, const SortedTri& b) {
            return AppConfig::depth_ascending ? (a.depth < b.depth) : (a.depth > b.depth);
        });
    }
}
