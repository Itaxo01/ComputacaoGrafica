#ifndef RENDERER_H
#define RENDERER_H

#include "imgui.h"
#include "Viewport.hpp"
#include "DisplayFile.hpp"
#include "GeometryBuffer.hpp"
#include "RenderedObject.hpp" // SortedTri
#include "log_app.h"
#include "Window.hpp"
#include "RendererCache.hpp"
#include "Framebuffer.hpp"
#include "Lighting.hpp"
#include "Shading.hpp"
#include "CudaPipeline.hpp"

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

    GeometryBuffer            geom;        // flat scene geometry: transformed + clipped
    std::vector<ObjectSlice>  slices;      // per-object render metadata (color/material/transform)
    std::vector<SortedTri>    sortedTris;  // all filled tris, depth-sorted for painter's
    std::vector<core::Light>  effectiveLights; // user lights + headlight, rebuilt per frame
    ShadingContext            shadeCtx;    // per-frame shading inputs for the rasterizer

    // CUDA path (no-op unless built with -DUSE_CUDA and a device is present).
    cuda::Context*       cudaCtx = nullptr;  // persistent device buffers
    bool                 prevUseCuda = false; // detect toggle → force geometry rebuild
    std::vector<ImU32>   cudaResolved;        // host landing buffer for the GPU image

    void RenderBackground();
    void DrawPreview();
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

    // Returns true (and refreshes the cache stamp) when the scene must be rebuilt
    // this frame; shared by the CPU and CUDA geometry paths.
    bool cacheDirty();
    // CUDA cache-miss geometry path: flatten + upload + run device geometry kernels.
    void GenerateGeometryCuda();
    cuda::GeomParams  makeGeomParams(bool shading);
    cuda::FrameParams makeFrameParams();

public:
    Renderer(DisplayFile& df, Viewport& v, Window& w, ExampleAppLog& log)
        : displayFile(df), viewport(v), window(w), log(log) {
        rendererCache = RendererCache(w.getWindowAttributes());
        cudaCtx = cuda::createContext(); // nullptr without -DUSE_CUDA / device
    }
    ~Renderer() { cuda::destroyContext(cudaCtx); }

    void notifyTransformation();
    void render();
};

#endif
