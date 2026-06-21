#include "PolygonFactory.hpp"
#include "Triangulate.hpp"

namespace core {

PolygonFactory::PolygonFactory(const std::string& name, const std::vector<core::Point>& pts, bool filled, ImU32 color)
    : name_(name), pts_(pts), filled_(filled), color_(color) {}

Object PolygonFactory::build() {
    Object obj;
    obj.name = name_;
    obj.type = ObjectType::POLYGON;
    obj.material.color = color_;
    obj.material.filled = filled_;
    obj.mesh->vertices = pts_;

    uint32_t n = (uint32_t)pts_.size();
    for (uint32_t i = 0; i < n; i++)
        obj.mesh->line_indices.push_back({i, (i + 1) % n});

    if (filled_) {
        std::vector<ImVec2> verts;
        verts.reserve(pts_.size());
        for (const auto& p : pts_) verts.push_back(ImVec2(p.x, p.y));
        auto tris = triangulate(verts);
        for (int i = 0; i < (int)tris.size(); i += 3)
            obj.mesh->tri_indices.emplace_back(
                (uint32_t)tris[i], (uint32_t)tris[i + 1], (uint32_t)tris[i + 2]);
    }
    return obj;
}

} // namespace core
