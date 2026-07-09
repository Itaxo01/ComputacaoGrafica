#pragma once
#include "Framebuffer.hpp"
#include "Shading.hpp"
#include "PipelineStages.hpp" // SortedTri, rasterizeTriangleInto

// CPU rasterization primitives. All coordinates are in framebuffer pixel space
// (already scaled by the SSAA factor). No OpenGL is used — pixels go straight into
// the Framebuffer, presented as a texture later.
//
// Each primitive restricts its writes to rows [y_lo, y_hi); the Renderer's banded
// parallel loop passes its band's row range so threads never touch the same pixel.

// Fill one depth-sorted triangle. The per-pixel coverage/shading is the shared
// rasterizeTriangleInto() (PipelineStages.hpp, also used by the CUDA rasterizer);
// the commit here writes into the Framebuffer — depth-tested when `depth_test` is
// set (z-buffer), otherwise overwrite (painter's order). `depth_less` selects the
// nearer direction. The triangle's bbox is clamped to valid pixels, so the writes
// are unchecked on the hot path.
void DrawSortedTri(Framebuffer& fb, const SortedTri& t, const ShadingContext& sctx,
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
