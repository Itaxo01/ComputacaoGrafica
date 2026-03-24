#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "imgui.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <math.h> // fmodf

#include "log_app.h"
#include "EntityManager.hpp"
#include "Shape.hpp"

class Window; // Forward declaration

class Viewport {
private:
    EntityManager entityManager;
    Window *window = nullptr;
    ExampleAppLog log;
    
    ImVec2 canvas_p0;
    ImVec2 canvas_p1;
    ImVec2 canvas_sz;
    ImDrawList* draw_list;
    
    core::ShapeType mode = core::ShapeType::POINT;
    // object creation
    ImVector<ImVec2> points;
    bool enable_object_creation = false;
    int e = 0;
    char obj_name[16] = "DEFAULT_NAME";

    std::vector<std::pair<float, float>> ImVecToVec(ImVector<ImVec2> &p);
    void HandleLeftClick();
    void HandleRightDragging(); 
    void HandleScroll(const float delta);

public:
    Viewport(EntityManager &em) : entityManager(em) {};
    //void SetWindow(Window &w) {window = w;}
    void AddGraphicObject();
    
    
    void DrawViewportWindow();
    void DrawCreateObjectWindow();
    void DrawLogWindow();
    // Para ficar melhor de manter, cada box da interface terá sua função de criação, run só chama todas elas.
    void run();

    void setWindow(Window *w){window = w;}

    ImVec2 GetViewportSize();
    ImVec2 GetCanvasSize() {return canvas_sz;};
    std::pair<ImVec2, ImVec2> GetCanvasP() {return std::make_pair(canvas_p0, canvas_p1);}
    ImDrawList* GetDrawList() {return draw_list;}
};

#endif // VIEWPORT_HPP
