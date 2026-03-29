#ifndef RENDERER_H
#define RENDERER_H

#include "imgui.h"
#include "Viewport.hpp"
#include "DisplayFile.hpp"
#include "Line.hpp"
#include "Point.hpp"
#include "Wireframe.hpp"
#include "log_app.h"
#include "Window.hpp"

class Renderer {
private:
    DisplayFile &displayFile;
    Viewport &viewport;
    Window &window;
    ImDrawList* draw_list = nullptr;

    ExampleAppLog log; // REMOVER DEPOIS

    void RenderBackground();
    void DrawObject(const core::Point &point);
    void DrawObject(const core::Line &line);
    void DrawObject(const core::Wireframe &wireframe);
    void renderName(const core::Shape &shape);
public:
    //Renderer(DisplayFile &df, Viewport &v, Window &w): displayFile(df), viewport(v), window(w) {}
    Renderer(DisplayFile &df, Viewport &v, Window &w): displayFile(df), viewport(v), window(w) {}
    void render();
};

#endif
