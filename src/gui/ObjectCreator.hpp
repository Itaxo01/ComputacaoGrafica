#ifndef OBJECT_CREATOR_HPP
#define OBJECT_CREATOR_HPP
#include "EntityManager.hpp"
#include "Shape.hpp"
#include "imgui.h"
#include "log_app.h"

class ObjectCreator{
    private:    
        core::ShapeType mode = core::ShapeType::POINT;
        std::vector<std::pair<float, float>> points;
        bool enable_object_creation = false;
        int e = 0;
        char obj_name[16] = "DEFAULT_NAME";
        int rgb_color[3] = {255, 255, 255}; // Stores RGB text inputs
        int object_color = IM_COL32_WHITE;
        ExampleAppLog &log;
        EntityManager &entityManager;
        
        void set_color(int r, int g, int b, int a){
            object_color = IM_COL32(r, g, b, a);
        }

    public:
        ObjectCreator(ExampleAppLog &log, EntityManager &em): log(log), entityManager(em){}
        
        void DrawWindow();
        void RegisterLeftClick(float x, float y); // received from the viewport
        void AddGraphicObject();
        
        void ImportFromFile(const char* file_path);
        void ExportToFile(const char* file_path);
};

#endif // OBJECT_CREATOR_HPP