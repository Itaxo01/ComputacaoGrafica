#include "ObjectCreator.hpp"
#include "gui/ObjectGUI.hpp"
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
    ExampleAppLog log; // o log usa singleton, vc pode criar dentro das classes
    DisplayFile displayFile; // Coleção de draws
    Viewport viewport(log); 
    Window programWindow(viewport);
    Renderer renderer(displayFile, viewport, programWindow, log);
    EntityManager entityManager(displayFile, renderer); // "view" para o display file
    // ao inves de passar para o construtor.
    
    ObjectCreator objectCreator(log, entityManager);

    /*
    ObjectListener objectListener(entityManager);
    GuiController guiController(entityManager, programWindow, viewport, objectCreator, log, objectListener);
    */

    ObjectController objectController(entityManager);
    ObjectGUI objectGUI(entityManager, objectController);
    GuiController guiController(entityManager, programWindow, viewport, objectCreator, log, objectGUI);

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

        // FPS Overlay
        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        if (ImGui::Begin("FPS Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
            float current_fps = ImGui::GetIO().DeltaTime > 0.0f ? 1.0f / ImGui::GetIO().DeltaTime : 0.0f;
            ImGui::Text("%.1f FPS (%.3f ms/frame)", current_fps, ImGui::GetIO().DeltaTime * 1000.0f);
        }
        ImGui::End();

        // Rendering
        ImGuiRender(window, clear_color);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
    CleanupImGui(window);
    return 0;
}
