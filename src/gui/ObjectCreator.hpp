#ifndef OBJECT_CREATOR_HPP
#define OBJECT_CREATOR_HPP
#include "EntityManager.hpp"
#include "Shape.hpp"
#include "log_app.h"

class ObjectCreator{
    private:    
        core::ShapeType mode = core::ShapeType::POINT;
        std::vector<std::pair<float, float>> points;
        bool enable_object_creation = false;
        int e = 0;
        char obj_name[16] = "DEFAULT_NAME";
        ExampleAppLog &log;
        EntityManager &entityManager;
        
    public:
        ObjectCreator(ExampleAppLog &log, EntityManager &em): log(log), entityManager(em){}
        
        void DrawWindow();
        void RegisterLeftClick(float x, float y); // received from the viewport
        void AddGraphicObject();

};

#endif // OBJECT_CREATOR_HPP