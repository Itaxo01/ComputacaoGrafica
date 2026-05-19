#include "EntityManager.hpp"
#include "Point.hpp"
#include "Mat4.hpp"
#include "Polygon.hpp"
#include "Shape.hpp"
#include "Wireframe.hpp"
#include "Curve2D.hpp"
#include <cassert>
#include <stdexcept>

void setName(core::Shape &s, const std::string &name){
    #ifndef DONT_DRAW_SHAPE_NAME
        s.name = name;
    #endif
}

void setColor(core::Shape &s, int object_color){
    #ifndef DONT_USE_OBJECT_COLOR
        s.object_color = object_color;
    #endif
}

void EntityManager::addPoint(const std::string &name, std::tuple<float, float, float> &t, int object_color){
    long long id = this->nextID(core::ShapeType::POINT);
    core::Point p(t);
    setName(p, name);
    setColor(p, object_color);
    displayFile.add(p, name, id);
}

void EntityManager::addLine(const std::string &name, std::tuple<float, float, float> &t1, std::tuple<float, float, float> &t2, int object_color){
    long long id = this->nextID(core::ShapeType::LINE);
    core::Point p1(t1); core::Point p2(t2);
    core::Line l(p1, p2);
    setName(l, name);
    setColor(l, object_color);
    
    displayFile.add(l, name, id);
}

void EntityManager::addWireframe(const std::string &name, std::vector<std::tuple<float, float, float>> &vp, int object_color) {
    long long id = this->nextID(core::ShapeType::WIREFRAME);
    std::vector<core::Point> core_vp;
    core_vp.reserve(vp.size()); // small optimization
    for (const auto &p : vp) {
        core_vp.emplace_back(p); 
    }
    core::Wireframe w(core_vp);
    setName(w, name);
    setColor(w, object_color);

    displayFile.add(w, name, id);
}

void EntityManager::addPolygon(const std::string &name, std::vector<std::tuple<float, float, float>> &vp, bool filled, int object_color){
    long long id = this->nextID(core::ShapeType::POLYGON);
    std::vector<core::Point> core_vp;
    core_vp.reserve(vp.size()); // small optimization
    for (const auto &p : vp) {
        core_vp.emplace_back(p); 
    }
    core::Polygon p(core_vp, filled);
    setName(p, name);
    setColor(p, object_color);

    displayFile.add(p, name, id);
}

void EntityManager::addCurve2D(const std::string &name, std::vector<std::tuple<float, float, float>> &vp, int object_color, int smoothness, int method) {
    long long id = this->nextID(core::ShapeType::CURVE2D);
    std::vector<core::Point> core_vp;
    core_vp.reserve(vp.size());
    for (const auto &p : vp) {
        core_vp.emplace_back(p);
    }
    core::Curve2D p(core_vp, smoothness, method);
    setName(p, name);
    setColor(p, object_color);

    displayFile.add(p, name, id);
}

void EntityManager::add(const std::string &name, std::vector<std::tuple<float, float, float>> &p, core::ShapeType &type, bool filled, int object_color, int smoothness, int method) {
    switch(type){
        case core::ShapeType::POINT: return addPoint(name, p[0], object_color);
        case core::ShapeType::LINE: return addLine(name, p[0], p[1], object_color);
        case core::ShapeType::WIREFRAME: return addWireframe(name, p, object_color);
        case core::ShapeType::POLYGON: return addPolygon(name, p, filled, object_color);
        case core::ShapeType::CURVE2D: return addCurve2D(name, p, object_color, smoothness, method);
        default: throw std::runtime_error("Undefined type at EntityManager add\n");
    }
}

void EntityManager::add(const bool generate_name, std::vector<std::tuple<float, float, float>> &p, core::ShapeType &type, bool filled, int object_color, int smoothness, int method){
    assert(generate_name == true);
    std::string name = getName(type, currentId);
    return add(name, p, type, filled, object_color, smoothness, method);
}

core::ObjectDetails EntityManager::GetObjectDetails(long long real_id) const {
    const auto& hash_id = getHashID();
    auto p_it = hash_id.find(real_id);
    if(p_it == hash_id.end()) return core::ObjectDetails{"Not found", "", "", "", ""};

    core::ShapeType type = (core::ShapeType)(real_id%10);
    auto &[hash_key, idpair] = *p_it;
    int list_id = idpair.first;
    long long fake_id = real_id/10;
    switch(type){
        case core::ShapeType::POINT: return displayFile.getPoint(list_id).GetObjectDetails(fake_id);
        case core::ShapeType::LINE: return displayFile.getLine(list_id).GetObjectDetails(fake_id);
        case core::ShapeType::WIREFRAME: return displayFile.getWireframe(list_id).GetObjectDetails(fake_id);
        case core::ShapeType::POLYGON: return displayFile.getPolygon(list_id).GetObjectDetails(fake_id);
        case core::ShapeType::CURVE2D: return displayFile.getCurve2D(list_id).GetObjectDetails(fake_id);
        default: return core::ObjectDetails{"Undefined", "", "", "", ""};
    }
}

std::vector<std::tuple<float,float,float>> EntityManager::GetObjectRawPoints(long long real_id) const {
    const auto& hmap = getHashID();
    auto it = hmap.find(real_id);
    if (it == hmap.end()) return {};
    int list_id = it->second.first;
    core::ShapeType type = (core::ShapeType)(real_id % 10);

    std::vector<std::tuple<float,float,float>> result;
    auto push = [&](const core::Point& p) { result.emplace_back(p.x, p.y, p.z); };

    switch (type) {
        case core::ShapeType::POINT:     push(displayFile.getPoint(list_id)); break;
        case core::ShapeType::LINE:      { auto& l = displayFile.getLine(list_id); push(l.a); push(l.b); break; }
        case core::ShapeType::WIREFRAME: for (auto& p : displayFile.getWireframe(list_id).points)        push(p); break;
        case core::ShapeType::POLYGON:   for (auto& p : displayFile.getPolygon(list_id).points)          push(p); break;
        case core::ShapeType::CURVE2D:   for (auto& p : displayFile.getCurve2D(list_id).control_points)  push(p); break;
        default: break;
    }
    return result;
}

void EntityManager::UpdateObjectPoints(long long real_id,
                                        const std::vector<std::tuple<float,float,float>>& new_pts,
                                        core::ShapeType new_type, int new_method, bool new_filled) {
    if (getHashID().find(real_id) == getHashID().end()) return;

    core::Shape& shape = displayFile.getShape(real_id);
    std::string name  = shape.name;
    int color         = shape.object_color;
    int smoothness    = 50;
    if (shape.type == core::ShapeType::CURVE2D)
        smoothness = static_cast<core::Curve2D&>(shape).smoothness;

    displayFile.remove(real_id);
    auto pts = new_pts;
    add(name, pts, new_type, new_filled, color, smoothness, new_method);
}

void EntityManager::ApplyTransformation(long long real_id, const core::mat4& matrix){
    core::Shape &shape = displayFile.getShape(real_id);
    switch(shape.type){
        case core::ShapeType::POINT: {
            core::Point &p = static_cast<core::Point&>(shape);
            core::Point k = matrix*p;
            p.x = k.x, p.y = k.y, p.z = k.z; // Para não modificar os outros atributos do ponto.
            break;
        }
        case core::ShapeType::LINE: {
            core::Line &line = static_cast<core::Line&>(shape);
            line.a = matrix*line.a;
            line.b = matrix*line.b;
            break;
        }
        case core::ShapeType::WIREFRAME: {
            core::Wireframe &wireframe = static_cast<core::Wireframe&>(shape);
            for(core::Point &p: wireframe.points){
                p = matrix*p;
            }
            break;
        }
        case core::ShapeType::POLYGON: {
            core::Polygon &polygon = static_cast<core::Polygon&>(shape);
            for(core::Point &p: polygon.points){
                p = matrix*p;
            }
            break;
        }
        case core::ShapeType::CURVE2D: {
            core::Curve2D &Curve2D = static_cast<core::Curve2D&>(shape);
            for(core::Point &p: Curve2D.points){
                p = matrix*p;
            }
            break;
        }
        default: throw std::runtime_error("Invalid object at Entity Manager ApplyTransformation\n");
    }
    renderer.notifyTransformation();
}