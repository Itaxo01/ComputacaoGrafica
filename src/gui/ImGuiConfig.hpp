#ifndef IMGUI_CONFIG_HPP
#define IMGUI_CONFIG_HPP

// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp


#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include "imgui.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif


#if IMGUI_VERSION_NUM >= 19263
namespace ImGui { extern IMGUI_API void DemoMarker(const char* file, int line, const char* section); }
#define IMGUI_DEMO_MARKER(section)  do { ImGui::DemoMarker("imgui_demo.cpp", __LINE__, section); } while (0)
#endif

// Initialize GLFW and return the window and the determined GLSL version string
GLFWwindow* InitializeGLFW(const char*& out_glsl_version);

// Initialize ImGui, contexts, and backends
void InitializeImGui(GLFWwindow* window, const char* glsl_version);

// Cleanup all ImGui and GLFW related constructs
void CleanupImGui(GLFWwindow* window);

void ImGuiRender(GLFWwindow *window, ImVec4 &clear_color);
void startImGuiFrame();

#endif // IMGUI_CONFIG_HPP