#ifndef RENDERER_H
#define RENDERER_H

#include "imgui.h"
#include "Viewport.hpp"
#include "DisplayFile.hpp"
#include "Object.hpp"
#include "log_app.h"
#include "Window.hpp"
#include "RendererCache.hpp"

class Renderer {
private:
    DisplayFile& displayFile;
    Viewport&    viewport;
    Window&      window;
    ImDrawList*  draw_list = nullptr;
    RendererCache rendererCache;
    bool refresh_cache = false;
    ExampleAppLog& log;

    std::vector<core::Object> drawObjects; // working copy: transformed + clipped

    void RenderBackground();
    void DrawPreview();
    void DrawObject(const core::Object& obj);
    void draw_name_if_visible(const std::string& name, const core::Point& anchor);
    void ApplyClipping();
    void ApplyViewportTransform();
    void ApplyNCSTransform();
    void GenerateDrawList();

public:
    Renderer(DisplayFile& df, Viewport& v, Window& w, ExampleAppLog& log)
        : displayFile(df), viewport(v), window(w), log(log) {
        rendererCache = RendererCache(w.getWindowAttributes());
    }

    void notifyTransformation();
    void render();
};

#endif
