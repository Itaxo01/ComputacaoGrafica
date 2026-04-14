#include "EntityManager.hpp"
#include "Point.hpp"
#include "Mat4.hpp"
#include "Shape.hpp"
#include "Wireframe.hpp"
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

void EntityManager::add(const std::string &name, std::vector<std::tuple<float, float, float>> &p, int object_color) {
    if(p.size() == 1) return addPoint(name, p[0], object_color);
    if(p.size() == 2) return addLine(name, p[0], p[1], object_color);
    if(p.size() >= 3) return addWireframe(name, p, object_color);
}

void EntityManager::add(const bool generate_name, std::vector<std::tuple<float, float, float>> &p, int object_color){
    assert(generate_name == true);
    std::string name = getName(getType(p), currentId);
    return add(name, p, object_color);    
}

core::ObjectDetails EntityManager::GetObjectDetails(long long real_id, bool p3d) const {
    const auto& hash_id = getHashID();
    auto p_it = hash_id.find(real_id);
    if(p_it == hash_id.end()) return core::ObjectDetails{"Not found", "", "", "", ""};

    core::ShapeType type = (core::ShapeType)(real_id%10);
    auto &[hash_key, idpair] = *p_it;
    int list_id = idpair.first;
    long long fake_id = real_id/10;
    switch(type){
        case core::ShapeType::POINT: return displayFile.getPoint(list_id).GetObjectDetails(fake_id, p3d);
        case core::ShapeType::LINE: return displayFile.getLine(list_id).GetObjectDetails(fake_id, p3d);
        case core::ShapeType::WIREFRAME: return displayFile.getWireframe(list_id).GetObjectDetails(fake_id, p3d);
        // case core::ShapeType::POLYGON: return displayFile.getPolygon(list_id).GetObjectDetails(fake_id, p3d);
        default: return core::ObjectDetails{"Undefined", "", "", "", ""};
    }
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
        default: throw std::runtime_error("Invalid object at Entity Manager ApplyTransformation\n");
    }
    renderer.notifyTransformation();
}