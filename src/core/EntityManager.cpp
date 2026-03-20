#include "EntityManager.hpp"

void EntityManager::addPoint(const std::string &name, float x, float y){
    long long id = this->nextID(Types::POINT);
    core::Point p(x, y);
}

void EntityManager::addLine(const std::string &name, float x1, float y1, float x2, float y2){
    // TO DO
    return;
}

void EntityManager::addWireframe(const std::string &name, std::vector<std::pair<float, float>>){
    // TO DO
    return;
}

void EntityManager::add(const std::string &name, std::vector<std::pair<float, float>> &p) {
    if(p.size() == 1) return addPoint(name, p[0].first, p[0].second);
    if(p.size() == 2) return addLine(name, p[0].first, p[0].second, p[1].first, p[1].second);
    if(p.size() >= 3) return addWireframe(name, p);
}

