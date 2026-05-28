#include "RendererTransform.hpp"
#include "ParallelUtils.hpp"

void TransformObjectAndDoNCS(std::vector<RenderedObject>& dest,
                              const std::vector<core::Object>& src,
                              const core::mat4& ncs_mat) {
    dest.resize(src.size());
    cg_parallel_for_each(dest.begin(), dest.end(), [&](RenderedObject& ro) {
        size_t i = &ro - dest.data();
        const core::Object& obj = src[i];
        ro.type     = obj.type;
        ro.material = obj.material;
        ro.mesh.line_indices = obj.mesh->line_indices;
        ro.mesh.tri_indices  = obj.mesh->tri_indices;
        core::mat4 combined = ncs_mat * obj.transform;
        ro.mesh.vertices.resize(obj.mesh->vertices.size());
        for (size_t j = 0; j < obj.mesh->vertices.size(); ++j)
            ro.mesh.vertices[j] = combined * obj.mesh->vertices[j];
    });
}

void TransformToViewport(std::vector<RenderedObject>& objs, const Window& window, const ImVec2& offset) {
    cg_parallel_for_each(objs.begin(), objs.end(), [&](RenderedObject& obj) {
        for (auto& v : obj.mesh.vertices) {
            v = window.NCSToViewport(v);
            v.x += offset.x;
            v.y += offset.y;
        }
    });
}
