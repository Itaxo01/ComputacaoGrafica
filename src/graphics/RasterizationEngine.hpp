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
// agnostic, so both user polygons and culled meshes draw correctly. The
// barycentric weights computed here are the natural hook for z-buffer depth and
// attribute interpolation in the later shading steps.
void DrawTriangleFilled(Framebuffer& fb, const ImVec2& a, const ImVec2& b,
                        const ImVec2& c, ImU32 color, int y_lo, int y_hi);

// Bresenham line, stamped `thickness` px wide.
void DrawLine(Framebuffer& fb, float x0, float y0, float x1, float y1,
              ImU32 color, int thickness, int y_lo, int y_hi);

// Filled square of half-extent `half` centered on (x, y).
void DrawPoint(Framebuffer& fb, float x, float y, ImU32 color, int half,
               int y_lo, int y_hi);
