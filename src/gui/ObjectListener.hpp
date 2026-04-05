#ifndef OBJECT_LISTENER_HPP
#define OBJECT_LISTENER_HPP

#include "imgui.h"
//#include "DisplayFile.hpp"
#include "EntityManager.hpp"
#include <unordered_set>
#include "Viewport.hpp"

// A lógica de transform está também nessa box, talvez não devesse.
class ObjectListener {
private:
    EntityManager& entityManager;
    //vector<ManifestEntry> manifest; // INICIALIZAR NO CONSTRUTOR
    Viewport& viewport;
    std::unordered_set<long long> selected_ids;
    int last_selected_index = -1; // Used to calculate ranges for Shift+Click
    int current_page = 0;
    const int items_per_page = 20;

    // Helper to get string name from ShapeType
    const char* GetTypeName(core::ShapeType type);

    std::vector<char*> transform_buf_names;

    void DrawObjectList();
public:
    ObjectListener(EntityManager& em, Viewport &v) : entityManager(em), viewport(v) {}

    void DrawWindow();

    void DrawTransformCombination();
    void HandleAddScaling(float x, float y);
    void HandleAddTranslation(float x, float y);
    void HandleAddRotation(float x, float y, float angle);
};

#endif // OBJECT_LISTENER_HPP