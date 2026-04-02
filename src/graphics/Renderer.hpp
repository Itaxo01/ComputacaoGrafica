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
#include "RendererCache.hpp"

class Renderer {
private:
    DisplayFile &displayFile;
    Viewport &viewport;
    Window &window;
    ImDrawList* draw_list = nullptr;
    RendererCache rendererCache;

    ExampleAppLog log; // REMOVER DEPOIS

    std::vector<core::Point> drawPointList; // O Renderer guarda os objetos que de fato serão desenhados (Pós clipping).
    std::vector<core::Line> drawLineList;
    std::vector<core::Line> drawWireframeList; // Aqui estamos quebrando o wireframe em linhas no pré processamento.


    void RenderBackground();
    void DrawObject(const core::Point &point);
    void DrawObject(const core::Line &line);
    void DrawObject(const core::Wireframe &wireframe);
    void renderName(const core::Shape &shape);
    void ApplyClipping(const core::Point &wp0, const core::Point &wp1);
    void ApplyViewportTransform();
    void GenerateDrawList();
public:
    //Renderer(DisplayFile &df, Viewport &v, Window &w): displayFile(df), viewport(v), window(w) {}
    Renderer(DisplayFile &df, Viewport &v, Window &w): displayFile(df), viewport(v), window(w) {
        rendererCache = RendererCache(w.GetWorldMin(), w.GetWorldMax(), df.object_count);
    }
    void render();
};

#endif
