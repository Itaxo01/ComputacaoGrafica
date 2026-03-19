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

class Viewport {
private:
    ExampleAppLog log;
    void HandleLeftClick();
    void HandleRightDragging(); 
    void HandlePointButtonClick();
    void HandleLineButtonClick();
    void HandleWireframeButtonClick();
public:
    void run();
    ImVec2 GetViewportSize();
    ImDrawList* GetDrawList();
};

enum Mode {
    POINT,
    LINE,
    WIREFRAME
};