#include "EntityManager.hpp"
#include "Shape.hpp"
#include <cassert>

void setName(core::Shape &s, const std::string &name){
    #ifndef DONT_DRAW_SHAPE_NAME
        s.name = name;
    #endif
}

void EntityManager::addPoint(const std::string &name, float x, float y){
    long long id = this->nextID(core::ShapeType::POINT);
    core::Point p(x, y);
    setName(p, name);

    displayFile.add(p, name, id);
}

void EntityManager::addLine(const std::string &name, float x1, float y1, float x2, float y2){
    long long id = this->nextID(core::ShapeType::LINE);
    core::Point p1(x1, y1); core::Point p2(x2, y2);
    core::Line l(p1, p2);
    setName(l, name);
    
    displayFile.add(l, name, id);
}

void EntityManager::addWireframe(const std::string &name, std::vector<std::pair<float, float>> &vp) {
    long long id = this->nextID(core::ShapeType::WIREFRAME);
    std::vector<core::Point> core_vp;
    core_vp.reserve(vp.size()); // small optimization
    for (const auto &p : vp) {
        core_vp.emplace_back(p.first, p.second); 
    }
    core::Wireframe w(core_vp);
    setName(w, name);

    displayFile.add(w, name, id);
}

void EntityManager::add(const std::string &name, std::vector<std::pair<float, float>> &p) {
    if(p.size() == 1) return addPoint(name, p[0].first, p[0].second);
    if(p.size() == 2) return addLine(name, p[0].first, p[0].second, p[1].first, p[1].second);
    if(p.size() >= 3) return addWireframe(name, p);
}

void EntityManager::add(const bool generate_name, std::vector<std::pair<float, float>> &p){
    assert(generate_name == true);
    std::string name = getName(getType(p), currentId);
    return add(name, p);    
}

std::string EntityManager::GetObjectDetails(long long real_id, bool p3d) const {
    auto hash_id = getHashID();
    auto p_it = hash_id.find(real_id);
    if(p_it == hash_id.end()) return "Object not found";

    core::ShapeType type = (core::ShapeType)(real_id%10);
    auto &[hash_key, idpair] = *p_it;
    int list_id = idpair.first;
    long long fake_id = real_id/10;
    switch(type){
        case core::ShapeType::POINT: return displayFile.getPoint(list_id).to_string(fake_id, p3d);
        case core::ShapeType::LINE: return displayFile.getLine(list_id).to_string(fake_id, p3d);
        case core::ShapeType::WIREFRAME: return displayFile.getWireframe(list_id).to_string(fake_id, p3d);
        default: return "Type not defined";
    }
}