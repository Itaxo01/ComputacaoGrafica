#include "RendererTransform.hpp"
#include "ParallelUtils.hpp"

void TransformToNCS(std::vector<core::Object>& objs, const core::mat4& ncs_mat) {
    cg_parallel_for_each(objs.begin(), objs.end(), [&](core::Object& obj) {
        core::mat4 combined = ncs_mat * obj.transform;
        for (auto& v : obj.mesh->vertices)
            v = combined * v;
    });
}

void TransformToViewport(std::vector<core::Object>& objs, const Window& window, const ImVec2& offset) {
    cg_parallel_for_each(objs.begin(), objs.end(), [&](core::Object& obj) {
        for (auto& v : obj.mesh->vertices) {
            v = window.NCSToViewport(v);
            v.x += offset.x;
            v.y += offset.y;
        }
    });
}
