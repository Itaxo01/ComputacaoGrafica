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

    std::vector<char*> transform_buf_names;

    void DrawObjectList();
public:
    ObjectListener(EntityManager& em) : entityManager(em) {}

    void DrawWindow();
    void DrawTransformCombination();
    void HandleAddScaling(float x, float y);
    void HandleAddTranslation(float x, float y);
    void HandleAddRotation(float x, float y, float angle);
};

#endif // OBJECT_LISTENER_HPP