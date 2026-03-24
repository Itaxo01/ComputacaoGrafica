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
    ImVec2 canvas_p0;
    ImVec2 canvas_p1;
    ImVec2 canvas_sz;
    ImDrawList* draw_list;

    std::vector<std::pair<float, float>> ImVecToVec(ImVector<ImVec2> &p);
    void HandleLeftClick();
    void HandleRightDragging(); 
    void HandleScroll(const float delta);

public:
    Viewport(){};
    //void SetWindow(Window &w) {window = w;}
    void AddGraphicObject();
    
    void DrawWindow();
    

    void setWindow(Window *w){window = w;}

    ImVec2 GetViewportSize();
    ImVec2 GetCanvasSize() {return canvas_sz;};
    std::pair<ImVec2, ImVec2> GetCanvasP() {return std::make_pair(canvas_p0, canvas_p1);}
    ImDrawList* GetDrawList() {return draw_list;}
};

#endif // VIEWPORT_HPP
