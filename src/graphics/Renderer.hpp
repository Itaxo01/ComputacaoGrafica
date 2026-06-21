#ifndef RENDERER_H
#define RENDERER_H

#include "imgui.h"
#include "Viewport.hpp"
#include "DisplayFile.hpp"
#include "RenderedObject.hpp"
#include "log_app.h"
#include "Window.hpp"
#include "RendererCache.hpp"
#include "Framebuffer.hpp"

class Renderer {
private:
    DisplayFile& displayFile;
    Viewport&    viewport;
    Window&      window;
    ImDrawList*  draw_list = nullptr;
    RendererCache rendererCache;
    bool refresh_cache = false;
    ExampleAppLog& log;
    Framebuffer framebuffer; // CPU raster target, presented as a texture each frame
    int last_supersample = 0; // detects AppConfig::supersample changes to invalidate the cache

    std::vector<RenderedObject> drawObjects; // working copy: transformed + clipped
    std::vector<SortedTri>      sortedTris;  // all filled tris, depth-sorted for painter's

    void RenderBackground();
    void DrawPreview();
    // Rasterize one object's lines/points into the framebuffer, restricted to
    // rows [y_lo, y_hi) (the caller's band). Filled triangles go through the
    // globally depth-sorted sortedTris list, not here.
    void DrawObject(const RenderedObject& obj, int y_lo, int y_hi);
    // Clears + rasterizes the whole scene into `framebuffer`, parallelized over
    // horizontal bands (each band owns a disjoint row range, so no write races).
    void RasterizeFramebuffer();
    void draw_name_if_visible(const std::string& name, const core::Point& anchor);
    void ApplyClipping();
    void ApplyViewportTransform();
    // Everything before the 2D clip: object transform + projection to NCS, plus
    // near-plane clipping in the perspective path.
    void ProcessPreClipping();
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
