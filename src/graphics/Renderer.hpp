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
    bool refresh_cache = false;

    ExampleAppLog log; // REMOVER DEPOIS

    std::vector<core::Point> drawPointList; // O Renderer guarda os objetos que de fato serão desenhados (Pós clipping).
    std::vector<core::Line> drawLineList;
    std::vector<core::Wireframe> wireframeMiddleware;
    std::vector<core::Line> drawWireframeList;


    void RenderBackground();
    void DrawObject(const core::Point &point);
    void DrawObject(const core::Line &line);
    void DrawObject(const core::Wireframe &wireframe);

    #ifndef DONT_DRAW_SHAPE_NAME
        void draw_name_if_visible(const core::Shape &shape);
    #endif

    
    void ApplyClipping();
    void ApplyViewportTransform();
    void ApplyNCSTransform();
    void GenerateDrawList();
public:
    //Renderer(DisplayFile &df, Viewport &v, Window &w): displayFile(df), viewport(v), window(w) {}
    Renderer(DisplayFile &df, Viewport &v, Window &w): displayFile(df), viewport(v), window(w) {
        rendererCache = RendererCache(w.getWindowAttributes(), df.object_count);
    }
    void notifyTransformation();
    void render();
};

#endif
