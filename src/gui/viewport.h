#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <math.h> // fmodf

#include "log_app.h"
#include "EntityManager.hpp"
#include "Window.hpp"
#include "ShapeType.hpp"

class Viewport {
private:
    EntityManager entityManager;
    //Window &window = nullptr;
    ShapeType mode = ShapeType::POINT;
    ExampleAppLog log;
    ImVector<ImVec2> points;

    ImVec2 canvas_p0;
    ImVec2 canvas_p1;
    ImVec2 canvas_sz;
    ImDrawList* draw_list;

    // object creation
    bool enable_object_creation = false;
    int e = 0;
    char obj_name[16] = "DEFAULT_NAME";

    std::vector<std::pair<float, float>> ImVecToVec(ImVector<ImVec2> &p);
    void HandleLeftClick();
    void HandleRightDragging(); 

public:
    Viewport(EntityManager &em) : entityManager(em) {};
    //void SetWindow(Window &w) {window = w;}
    ImVec2 GetViewportSize();
    ImDrawList* GetDrawList() {return draw_list;}
    void AddGraphicObject();
    void run();
    std::pair<ImVec2, ImVec2> GetCanvasP() {return std::make_pair(canvas_p0, canvas_p1);}
    ImVec2 GetCanvasSize() {return canvas_sz;};
};

#endif // VIEWPORT_H
