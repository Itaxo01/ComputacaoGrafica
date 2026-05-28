#pragma once
#include "Object.hpp" // pulls in Mesh.hpp, Material.hpp, ObjectType

struct RenderedObject {
    core::ObjectType type     = core::ObjectType::NONE;
    core::Material   material = {};
    core::Mesh       mesh;    // by value — avoids shared_ptr alloc and pointer indirection
};
