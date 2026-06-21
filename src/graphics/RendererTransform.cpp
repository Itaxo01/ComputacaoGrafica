#include "RendererTransform.hpp"
#include "ParallelUtils.hpp"
#include "AppConfig.hpp"
#include <algorithm>

void TransformObjectAndDoNCS(std::vector<RenderedObject>& dest,
                              const std::vector<core::Object>& src,
                              const core::mat4& ncs_mat) {
    dest.resize(src.size());
    cg_parallel_for_each(dest.begin(), dest.end(), [&](RenderedObject& ro) {
        size_t i = &ro - dest.data();
        const core::Object& obj = src[i];
        ro.type     = obj.type;
        ro.color    = obj.material.color;
        ro.filled   = obj.material.filled;
        ro.mesh.line_indices = obj.mesh->line_indices;
        ro.mesh.tri_indices  = obj.mesh->tri_indices;
        core::mat4 combined = ncs_mat * obj.transform;
        ro.mesh.vertices.resize(obj.mesh->vertices.size());
        for (size_t j = 0; j < obj.mesh->vertices.size(); ++j)
            ro.mesh.vertices[j] = combined * obj.mesh->vertices[j];
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
            v = window.NCSToViewport(v);
            v.x *= scale;
            v.y *= scale;
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

            out.push_back({ ImVec2(va.x * scale, va.y * scale),
                            ImVec2(vb.x * scale, vb.y * scale),
                            ImVec2(vc.x * scale, vc.y * scale),
                            o.color, (A.z + B.z + C.z) / 3.0f });
        }
    }

    if (AppConfig::is3d && AppConfig::depth_sort) {
        std::sort(out.begin(), out.end(), [](const SortedTri& a, const SortedTri& b) {
            return AppConfig::depth_ascending ? (a.depth < b.depth) : (a.depth > b.depth);
        });
    }
}
