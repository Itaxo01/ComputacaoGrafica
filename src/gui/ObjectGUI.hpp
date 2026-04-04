#ifndef OBJECT_GUI_HPP
#define OBJECT_GUI_HPP

#include "imgui.h"
#include "EntityManager.hpp"
#include "MultipleSelectionList.hpp"
#include "ObjectController.hpp"
#include <unordered_set>

// A lógica de transform está também nessa box, talvez não devesse.
class ObjectGUI {
private:
    EntityManager& entityManager;
    ObjectController &objectController;
    MultipleSelectionList multipleSelectionList;
    std::unordered_set<long long> selected_ids;

    // Helper to get string name from ShapeType
    const char* GetTypeName(core::ShapeType type);

    void DrawObjectList();
    void DrawObjectDetails();
    inline void DrawAddScaling();
    inline void DrawAddTranslation();
    inline void DrawAddRotation();
public:
    ObjectGUI(EntityManager& em, ObjectController& oc) : entityManager(em), objectController(oc) {
        oc.SetSelectedIDs(selected_ids);
    }
    void DrawWindow();
    void DrawTransformCombination();
};

#endif // OBJECT_GUI_HPP