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

enum class Mode {
    POINT,
    LINE,
    WIREFRAME,
    NONE
};

class Viewport {
private:
    EntityManager entityManager;
    Mode mode = Mode::NONE;
    ExampleAppLog log;
    ImVector<ImVec2> points;

    std::vector<std::pair<float, float>> ImVecToVec(ImVector<ImVec2> &p);
    void HandleLeftClick();
    void HandleRightDragging(); 
    void HandlePointButtonClick();
    void HandleLineButtonClick();
    void HandleWireframeButtonClick();
    void HandleEnterButtonClick();
public:
    Viewport(EntityManager &em) : entityManager(em) {};
    ImVec2 GetViewportSize();
    ImDrawList* GetDrawList();
    void AddGraphicObject();
    void run();
};

#endif // VIEWPORT_H
