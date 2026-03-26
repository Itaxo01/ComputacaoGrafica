#ifndef OBJECT_LISTENER_HPP
#define OBJECT_LISTENER_HPP

#include "imgui.h"
#include "EntityManager.hpp"
#include <unordered_set>

#define ImGay ImGui

class ObjectListener {
private:
    EntityManager& entityManager;
    std::unordered_set<long long> selected_ids;
    int last_selected_index = -1; // Used to calculate ranges for Shift+Click

    // Helper to get string name from ShapeType
    const char* GetTypeName(core::ShapeType type);

    //float fsx = 0.0f, fsy = 0.0f; // get scaling input

    void DrawObjectList();
public:
    ObjectListener(EntityManager& em) : entityManager(em) {}

    void DrawWindow();
};

#endif // OBJECT_LISTENER_HPP