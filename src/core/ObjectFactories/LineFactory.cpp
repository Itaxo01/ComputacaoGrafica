#include "LineFactory.hpp"

namespace core {

LineFactory::LineFactory(const std::string& name, core::Point a, core::Point b, ImU32 color)
    : name_(name), a_(a), b_(b), color_(color) {}

Object LineFactory::build() {
    Object obj;
    obj.name = name_;
    obj.type = ObjectType::LINE;
    obj.material.color = color_;
    obj.mesh->vertices = {a_, b_};
    obj.mesh->line_indices = {{0, 1}};
    return obj;
}

} // namespace core
