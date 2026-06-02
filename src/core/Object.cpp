#include "Object.hpp"
#include <stdexcept>
#include <cmath>

namespace core {

// ── Object methods ────────────────────────────────────────────────────────────

core::Point Object::anchorPoint() const {
    if (mesh->vertices.empty()) return transform * core::Point(0, 0, 0);
    core::Point best = transform * mesh->vertices.front();
    for (size_t i = 1; i < mesh->vertices.size(); i++)
        best = max_y(best, transform * mesh->vertices[i]);
    return best;
}

core::Point Object::centerPoint() const {
    if (mesh->vertices.empty()) return transform * core::Point(0, 0, 0);
    float sx = 0, sy = 0, sz = 0;
    for (const auto& v : mesh->vertices) { sx += v.x; sy += v.y; sz += v.z; }
    float n = (float)mesh->vertices.size();
    return transform * core::Point(sx/n, sy/n, sz/n);
}

static std::string colorString(ImU32 col) {
    unsigned int uc = (unsigned int)col;
    int r = (uc >>  0) & 0xFF;
    int g = (uc >>  8) & 0xFF;
    int b = (uc >> 16) & 0xFF;
    return "[" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + "]";
}

ObjectDetails Object::GetObjectDetails() const {
    ObjectDetails d;
    d.type  = getTypeName(type);
    d.id    = std::to_string(id);
    d.name  = name;
    d.color = colorString(material.color);

    std::string pts = "[";
    for (size_t i = 0; i < mesh->vertices.size(); ++i) {
        pts += (transform * mesh->vertices[i]).coords();
        if (i + 1 < mesh->vertices.size()) pts += "\n";
    }
    pts += "]";
    if (type == ObjectType::POLYGON)
        pts += material.filled ? " (filled)" : " (outline)";
    d.points = pts;
    return d;
}

std::vector<std::tuple<float,float,float>> Object::GetRawPoints() const {
    std::vector<std::tuple<float,float,float>> result;
    result.reserve(mesh->vertices.size());
    for (const auto& v : mesh->vertices) {
        core::Point w = transform * v;
        result.emplace_back(w.x, w.y, w.z);
    }
    return result;
}

void Object::ApplyTransformation(const mat4& m) {
    transform = m * transform;
}

// ── getTypeName ───────────────────────────────────────────────────────────────

const char* getTypeName(ObjectType type) {
    switch (type) {
        case ObjectType::POINT:    return "Point";
        case ObjectType::LINE:     return "Line";
        case ObjectType::WIREFRAME:return "Wireframe";
        case ObjectType::POLYGON:  return "Polygon";
        case ObjectType::CURVE2D:  return "Curve2D";
        case ObjectType::SURFACE:  return "Surface";
        case ObjectType::MESH:     return "Mesh";
        default:                  return "Unknown";
    }
}

bool typeAvailableInMode(ObjectType type, bool is3d) {
    switch (type) {
        case ObjectType::POINT:
        case ObjectType::LINE:
        case ObjectType::WIREFRAME:
        case ObjectType::POLYGON:   return true;
        case ObjectType::CURVE2D:   return !is3d;
        case ObjectType::SURFACE:   return is3d;
        default:                    return false;
    }
}

} // namespace core
