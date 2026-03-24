#include "ObjectCreator.hpp"
#include "ObjectListener.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <math.h> // fmodf

// NOSSOS IMPORTS
#include "ObjectCreator.hpp"
#include "GuiController.hpp"
#include "Viewport.hpp"
#include "Renderer.hpp"
#include "log_app.h"
#include "Window.hpp"
#include "EntityManager.hpp"

#include "gui/ImGuiConfig.hpp" // Configs do imgui foram pra cá

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

int main(int, char**) {
    // 1. Initialize window
    const char* glsl_version = nullptr;
    GLFWwindow *window = InitializeGLFW(glsl_version);
    if(!window) return 1;

    // 2. Initialize ImGui
    InitializeImGui(window, glsl_version);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


    // 3. Inicializa nossas classes
    DisplayFile displayFile; // Coleção de draws
    EntityManager entityManager(displayFile); // "view" para o display file
    Viewport viewport; 
    Window programWindow(viewport);
    Renderer renderer(displayFile, viewport, programWindow);
    ExampleAppLog log;
    
    ObjectCreator objectCreator(log, entityManager);
    ObjectListener objectListener(entityManager);
    GuiController guiController(entityManager, programWindow, viewport, objectCreator, log, objectListener);

    // Main loop
#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        startImGuiFrame();

        // Nossas janelas rodam aqui!
        guiController.run();
        renderer.render();

        // Rendering
        ImGuiRender(window, clear_color);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
    CleanupImGui(window);
    return 0;
}
