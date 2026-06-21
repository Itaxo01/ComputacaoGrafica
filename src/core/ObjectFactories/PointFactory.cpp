#include "PointFactory.hpp"

namespace core {

PointFactory::PointFactory(const std::string& name, float x, float y, float z, ImU32 color)
    : name_(name), x_(x), y_(y), z_(z), color_(color) {}

Object PointFactory::build() {
    Object obj;
    obj.name = name_;
    obj.type = ObjectType::POINT;
    obj.material.color = color_;
    obj.mesh->vertices.push_back(core::Point(x_, y_, z_));
    return obj;
}

} // namespace core
