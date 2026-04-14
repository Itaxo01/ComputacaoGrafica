#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "imgui.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <math.h> // fmodf

class Viewport {
private:
    ImVec2 canvas_p0;
    ImVec2 canvas_p1;
    ImVec2 canvas_sz;
    ImDrawList* draw_list;
    const float offset = 15.0f;

    bool is_hovered = false;
    bool is_active = false;
public:
    Viewport() = default;
    
    bool show_axes = true;
    bool show_grid = true;
    bool show_axis_coordinates = true;
    bool is3d = false;
    int  clipping_mode = 0;
    
    void DrawWindow();
    bool IsHovered() const {return is_hovered;}
    bool IsActive() const {return is_active;}

    ImVec2 GetCanvasSize() {return canvas_sz;};
    std::pair<ImVec2, ImVec2> GetCanvasP() {return std::make_pair(canvas_p0, canvas_p1);}
    ImDrawList* GetDrawList() {return draw_list;}

    // 0 Liang Barsky, 1 South alguma coisa
    int GetClippingMode() const {return clipping_mode;}
};

#endif // VIEWPORT_HPP
