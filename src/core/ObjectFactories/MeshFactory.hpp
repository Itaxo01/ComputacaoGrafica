#pragma once
#include "ObjectFactory.hpp"

namespace core {
    // Thin factory that adopts an already-built mesh Object. The OBJ importer
    // assembles the full Object (geometry + faces + materials) itself, then hands
    // it to EntityManager::add through this factory to keep the creation path
    // uniform with the other primitives.
    class MeshFactory : public ObjectFactory {
        Object obj_;
    public:
        explicit MeshFactory(Object obj) : obj_(std::move(obj)) {}
        Object build() override { return std::move(obj_); }
    };
}
