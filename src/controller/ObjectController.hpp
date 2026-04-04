#ifndef OBJECT_CONTROLLER_HPP
#define OBJECT_CONTROLLER_HPP

#include "EntityManager.hpp"

class ObjectController {
private:
    EntityManager& entityManager;
public:
    ObjectController(EntityManager& em) : entityManager(em) {}

    void HandleAddScaling(float x, float y);
    void HandleAddTranslation(float x, float y);
    void HandleAddRotation(float x, float y, float angle);
};

#endif // OBJECT_CONTROLLER_HPP