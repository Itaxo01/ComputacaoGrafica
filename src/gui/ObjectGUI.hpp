#ifndef OBJECT_GUI_HPP
#define OBJECT_GUI_HPP

#include "imgui.h"
#include "EntityManager.hpp"
#include "MultipleSelectionList.hpp"
#include "ObjectController.hpp"
#include "ObjectCreatorText.hpp"
#include "log_app.h"
#include <unordered_set>

// A lógica de transform está também nessa box, talvez não devesse.
class ObjectGUI {
private:
    EntityManager& entityManager;
    ObjectController &objectController;
    ExampleAppLog& log;
    MultipleSelectionList multipleSelectionList;
    MultipleSelectionList transformationsList;
    std::unordered_set<long long> selected_ids;

    ObjectCreatorText point_editor;
    long long editing_id = -1; // id of object currently being edited (-1 = none)

    // Helper to get string name from ObjectType
    const char* GetTypeName(core::ObjectType type);

    bool view_matrix_popup_open = false; // State variable for popup
    core::mat4 matrix_to_view; // Matrix to display in popup

    void DrawObjectList();
    void DrawObjectDetails();
    inline void DrawAddScaling();
    inline void DrawAddTranslation();
    inline void DrawAddRotation();
    void DrawTransformImport();
public:
    ObjectGUI(EntityManager& em, ObjectController& oc, ExampleAppLog& log)
        : entityManager(em), objectController(oc), log(log),
          point_editor(log, "Edit Points") {
        oc.SetSelectedIDs(selected_ids);
    }
    void DrawWindow();
    void DrawTransformCombination();
};

#endif // OBJECT_GUI_HPP