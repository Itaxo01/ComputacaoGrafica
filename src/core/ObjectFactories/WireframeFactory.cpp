#include "WireframeFactory.hpp"

namespace core {

WireframeFactory::WireframeFactory(const std::string& name, const std::vector<core::Point>& pts, ImU32 color)
    : name_(name), pts_(pts), color_(color) {}

Object WireframeFactory::build() {
    Object obj;
    obj.name = name_;
    obj.type = ObjectType::WIREFRAME;
    obj.material.color = color_;
    obj.mesh->vertices = pts_;
    for (uint32_t i = 0; i + 1 < (uint32_t)pts_.size(); i++)
        obj.mesh->line_indices.push_back({i, i + 1});
    return obj;
}

} // namespace core
