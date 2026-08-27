#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "imgui.h"
#include "log_app.h"
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
    ExampleAppLog &log;

    bool is_hovered = false;
    bool is_active = false;
public:
    Viewport(ExampleAppLog &log): log(log) {};

    void DrawWindow();
    bool IsHovered() const {return is_hovered;}
    bool IsActive() const {return is_active;}

    // Set the canvas rectangle directly, without an ImGui frame. DrawWindow()
    // is the normal path and measures this from the Viewport window; the
    // headless benchmark (src/bench) has no ImGui context and feeds a fixed
    // rectangle instead, so that the Window -> NCS -> viewport mapping is the
    // same one the GUI produces. Nothing in the application calls this.
    void SetCanvas(const ImVec2 &p0, const ImVec2 &p1) {
        canvas_p0 = p0;
        canvas_p1 = p1;
        canvas_sz = ImVec2(p1.x - p0.x, p1.y - p0.y);
    }

    ImVec2 GetCanvasSize() {return canvas_sz;};
    std::pair<ImVec2, ImVec2> GetCanvasP() const {return std::make_pair(canvas_p0, canvas_p1);}
    ImDrawList* GetDrawList() {return draw_list;}
};

#endif // VIEWPORT_HPP
