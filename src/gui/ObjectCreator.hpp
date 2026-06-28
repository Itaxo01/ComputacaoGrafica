#ifndef OBJECT_CREATOR_HPP
#define OBJECT_CREATOR_HPP
#include "EntityManager.hpp"
#include "ObjectCreatorText.hpp"
#include "Object.hpp"
#include "imgui.h"
#include "log_app.h"

class ObjectCreator{
    private:
        core::ObjectType mode = core::ObjectType::POINT;
        std::vector<std::tuple<float, float, float>> points;
        bool filled = false;       // only meaningful in polygon mode
        int curve_smoothness = 50; // points per segment for Curve2D
        int method = 0;            // 0=Bezier, 1=B-Spline (Curve2D / Surface)
        int surf_rows = 0;         // surface control-grid shape (from the text modal)
        int surf_cols = 0;
        int surf_eval = SURF_FORWARD_DIFF; // surface tessellation technique
        char obj_name[64] = "";    // empty = auto-generate
        float color_f[3] = {1.0f, 1.0f, 1.0f};
        int object_color = IM_COL32_WHITE;
        ExampleAppLog &log;
        EntityManager &entityManager;
        ObjectCreatorText text_creator;

        void set_color(float r, float g, float b){
            object_color = IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
        }

    public:
        ObjectCreator(ExampleAppLog &log, EntityManager &em)
            : log(log), entityManager(em), text_creator(log) {}

        void DrawWindow();
        void RegisterLeftClick(float x, float y, float z = 0.0f);
        void AddGraphicObject();
        // Build from the current in-progress `points` using an explicit type, so
        // the text modal can create an object whose mode differs from the active
        // radio (e.g. a 2D Curve while the app is in 3D mode).
        void AddGraphicObjectAs(core::ObjectType type, int method, bool filled);
        void CloseShape();    // finish wireframe/polygon — called by Enter or double-click
        void CancelCreation();// discard in-progress points — called by Escape

        // Read-only state for GuiController to draw the in-progress preview
        const std::vector<std::tuple<float, float, float>>& getInProgressPoints() const { return points; }
        core::ObjectType getMode() const { return mode; }

        void ImportFromFile(const char* file_path);
        void ExportToFile(const char* file_path);
};

#endif // OBJECT_CREATOR_HPP
