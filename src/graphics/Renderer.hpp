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

    ExampleAppLog &log;

    std::vector<core::Point> drawPointList;
    std::vector<core::Line> drawLineList;
    std::vector<core::Wireframe> wireframeMiddleware;
    std::vector<core::Line> drawWireframeList;
    std::vector<core::Polygon> drawPolygonList;
    std::vector<core::Curve2D> Curve2DMiddleware;
    std::vector<core::Line> drawCurve2DList;

    void RenderBackground();
    void DrawAllParallel();
    void DrawPreview();
    void DrawObject(const core::Point &point);
    void DrawObject(const core::Line &line);
    void DrawObject(const core::Wireframe &wireframe);
    void DrawObject(const core::Polygon &polygon);
    void DrawObject(const core::Curve2D &Curve2D);

    #ifndef DONT_DRAW_SHAPE_NAME
        void draw_name_if_visible(const core::Shape &shape);
    #endif

    void ApplyClipping();
    void ApplyViewportTransform();
    void ApplyNCSTransform();
    void GenerateDrawList();
public:
    Renderer(DisplayFile &df, Viewport &v, Window &w, ExampleAppLog &log): displayFile(df), viewport(v), window(w), log(log) {
        rendererCache = RendererCache(w.getWindowAttributes(), df.object_count);
    }
    static std::vector<int> triangulate(std::vector <ImVec2> poly); // refatorar depois
    void notifyTransformation();
    void render();
};

#endif
