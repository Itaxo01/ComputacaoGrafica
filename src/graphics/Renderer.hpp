#ifndef RENDERER_H
#define RENDERER_H

#include "imgui.h"
#include "viewport.h"
#include "DisplayFile.hpp"
#include "Line.hpp"
#include "Point.hpp"
#include "Wireframe.hpp"
#include "log_app.h"

class Renderer {
private:
    DisplayFile &displayFile;
    Viewport &viewport;

    ExampleAppLog log; // REMOVER DEPOIS

    void RenderBackground();
    void DrawObject(core::Point point);
public:
    Renderer(DisplayFile &df, Viewport &v): displayFile(df), viewport(v) {}
    void render();
};

#endif
