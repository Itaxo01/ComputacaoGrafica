#ifndef OBJECT_CREATOR_HPP
#define OBJECT_CREATOR_HPP
#include "EntityManager.hpp"
#include "Shape.hpp"
#include "imgui.h"
#include "log_app.h"

class ObjectCreator{
    private:
        core::ShapeType mode = core::ShapeType::POINT;
        std::vector<std::tuple<float, float, float>> points;
        bool filled = false;       // only meaningful in polygon_mode; wired up when Polygon class is ready
        int e = 0;                 // radio button state: 0=Point 1=Line 2=Wireframe 3=Polygon
        char obj_name[64] = "";    // empty = auto-generate
        float color_f[3] = {1.0f, 1.0f, 1.0f};
        int object_color = IM_COL32_WHITE;
        ExampleAppLog &log;
        EntityManager &entityManager;

        void set_color(float r, float g, float b){
            object_color = IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
        }

    public:
        ObjectCreator(ExampleAppLog &log, EntityManager &em): log(log), entityManager(em){}

        void DrawWindow();
        void RegisterLeftClick(float x, float y, float z = 0.0f);
        void AddGraphicObject();
        void CloseShape();    // finish wireframe/polygon — called by Enter or double-click
        void CancelCreation();// discard in-progress points — called by Escape

        // Read-only state for GuiController to draw the in-progress preview
        const std::vector<std::tuple<float, float, float>>& getInProgressPoints() const { return points; }
        core::ShapeType getMode() const { return mode; }

        void ImportFromFile(const char* file_path);
        void ExportToFile(const char* file_path);
};

#endif // OBJECT_CREATOR_HPP
