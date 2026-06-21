#pragma once
#include "Framebuffer.hpp"

// CPU rasterization primitives. All coordinates are in framebuffer pixel space
// (i.e. already scaled by Framebuffer::SUPERSAMPLE). No OpenGL is used to draw —
// pixels go straight into the Framebuffer, which is later presented as a texture.
//
// The single-pixel write (draw_pixel) lives on the Framebuffer itself as
// SetPixel(). These are the higher-level primitives the Renderer calls.
//
// Each primitive restricts its writes to rows [y_lo, y_hi); the Renderer's
// banded parallel loop passes its band's row range so a primitive spanning
// several bands is split cleanly and threads never touch the same pixel.

// Edge-function (barycentric) fill over the triangle's bounding box. Winding-
// agnostic, so both user polygons and culled meshes draw correctly. When
// `depth_test` is set, per-pixel depth is interpolated from the vertex depths
// (za/zb/zc) via the barycentric weights and the fragment is depth-tested +
// written (SetPixelDepth). `depth_less` selects the nearer direction.
void DrawTriangleFilled(Framebuffer& fb, const ImVec2& a, const ImVec2& b,
                        const ImVec2& c, float za, float zb, float zc, ImU32 color,
                        bool depth_test, bool depth_less, int y_lo, int y_hi);

// Bresenham line, stamped `thickness` px wide. Depth (z0..z1) is interpolated
// linearly along the line; when `depth_test` is set, each pixel is depth-tested
// read-only (DepthPasses) so the line is hidden behind solids without writing depth.
void DrawLine(Framebuffer& fb, float x0, float y0, float z0, float x1, float y1, float z1,
              ImU32 color, int thickness, bool depth_test, bool depth_less,
              int y_lo, int y_hi);

// Filled square of half-extent `half` centered on (x, y), at depth z. Depth-tested
// read-only like DrawLine when `depth_test` is set.
void DrawPoint(Framebuffer& fb, float x, float y, float z, ImU32 color, int half,
               bool depth_test, bool depth_less, int y_lo, int y_hi);
