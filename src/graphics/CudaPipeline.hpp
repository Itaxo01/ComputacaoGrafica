#pragma once
#include <vector>
#include <cstdint>
#include "imgui.h"
#include "GeometryBuffer.hpp"
#include "Mat4.hpp"
#include "Point.hpp"
#include "Material.hpp" // core::Color3
#include "Light.hpp"

// Host-facing CUDA pipeline API. This header is PLAIN C++ (no CUDA types) so the
// regular g++ translation units (Renderer.cpp, ...) can include it. The real
// implementation lives in CudaPipeline.cu (compiled by nvcc, only in the `cuda`
// make target); a stub TU (CudaPipeline_stub.cpp) provides the symbols when the
// project is built WITHOUT -DUSE_CUDA, so `available()` returns false and the
// Renderer transparently falls back to the CPU pipeline.
namespace cuda {

// Everything the geometry kernels need, gathered once per cache-miss by the
// Renderer from Window / AppConfig. Mirrors the inputs of the CPU stages in
// RendererTransform.cpp / RendererClipping.cpp.
struct GeomParams {
    bool is3d = false, perspective = false, shading = false;
    core::mat4 vrc;          // GetVRCMatrix()            (perspective path)
    core::mat4 projScale;    // GetProjectionScaleMatrix()(perspective path)
    core::mat4 windowNcs;    // GetWindowNCSMatrix()      (ortho/2D path)
    float nearZ = 0.0f;      // GetNearPlaneZ()
    core::Point clipMin, clipMax; // getClipBoundsNCS()
    int   clippingMode = 0;  // AppConfig::clipping_mode
    bool  backfaceCull = false, cullCcw = true, depthSort = true,
          depthAscending = false, zBuffer = true;
    float canvasW = 0.0f, canvasH = 0.0f; // display resolution (NCSToViewport target)
    float scale = 1.0f;       // supersample factor (applied after NCSToViewport)
};

// Per-frame shading + rasterization inputs (read live; no geometry rebuild).
struct FrameParams {
    int   shadeMode = 0;     // Lighting::Mode (0 = none)
    core::Point  eye;
    core::Color3 ambient;
    std::vector<core::Light> lights;
    bool  zBuffer = true, depthLess = true;
    int   supersample = 1;
    int   displayW = 0, displayH = 0; // resolved image size (== framebuffer display dims)
};

// True only when built with -DUSE_CUDA AND a usable device is present.
bool available();

// Opaque, renderer-owned handle holding all persistent device buffers.
struct Context;
Context* createContext();
void     destroyContext(Context* ctx);

// Cache-miss path: upload the freshly-flattened (indexed) GeometryBuffer + slice
// table and run the whole geometry pipeline on the device (transform → near-clip →
// project → box-clip → sorted-tris → viewport). Results stay device-resident.
void processGeometry(Context* ctx, const GeometryBuffer& in,
                     const std::vector<ObjectSlice>& slices, const GeomParams& gp);

// Per-frame path: rasterize the device-resident geometry into the display-resolution
// image. If `glTexture` is non-zero and CUDA-OpenGL interop succeeds, the result is
// written straight into that GL texture (no host copy) and `*outUsedInterop` is set
// true. Otherwise the image is copied to `outResolved` (caller-owned,
// displayW*displayH ImU32s) for Framebuffer::PresentExternal and `*outUsedInterop`
// is false.
void rasterize(Context* ctx, const FrameParams& fp, unsigned glTexture,
               ImU32* outResolved, bool* outUsedInterop);

} // namespace cuda
