#include "EntityManager.hpp"

void EntityManager::addPoint(const std::string &name, float x, float y){
    long long id = this->nextID(Types::POINT);
    core::Point p(x, y);
}

void EntityManager::addLine(const std::string &name, float x1, float y1, float x2, float y2){

}

void EntityManager::addWireframe(const std::string &name, std::vector<std::pair<float, float>>){

}

