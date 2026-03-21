#ifndef RENDERER_H
#define RENDERER_H

#include "imgui.h"
#include "viewport.h"
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
    //Window &window;

    ExampleAppLog log; // REMOVER DEPOIS

    void RenderBackground();
    void DrawObject(core::Point point);
    void DrawObject(core::Line line);
    void DrawObject(core::Wireframe wireframe);
public:
    //Renderer(DisplayFile &df, Viewport &v, Window &w): displayFile(df), viewport(v), window(w) {}
    Renderer(DisplayFile &df, Viewport &v): displayFile(df), viewport(v) {}
    void render();
};

#endif
