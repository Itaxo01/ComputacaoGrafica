#pragma once
#include "Framebuffer.hpp"
#include "Shading.hpp"

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
// agnostic, so both user polygons and culled meshes draw correctly. Depth is
// interpolated from za/zb/zc; when `depth_test` is set the fragment is depth-
// tested + written. `depth_less` selects the nearer direction.
//
// Shading: when `sctx.mode != 0`, the per-pixel color is computed from the Phong
// model using the per-vertex world positions P[3], normals N[3] and material —
// Flat (one face color), Gouraud (interpolate 3 vertex colors) or Phong (per-
// pixel normal). When mode == 0, `flatColor` is used unchanged.
//
// The inner loop uses UNCHECKED framebuffer writes: the bbox is pre-clamped to
// valid pixels here, so no per-pixel bounds branch is needed on the hot path.
//
// bb_* is the triangle's pixel bounding box (floor/ceil of the vertex extents),
// passed in rather than recomputed: the banded rasterizer calls this for the same
// triangle from several bands, and the caller needs the box anyway to decide
// whether the triangle touches the band at all.
void DrawTriangleFilled(Framebuffer& fb, const ImVec2& a, const ImVec2& b,
                        const ImVec2& c, float za, float zb, float zc,
                        int bb_min_x, int bb_min_y, int bb_max_x, int bb_max_y,
                        const core::Point P[3], const core::Point N[3],
                        const ShadeMaterial& mat, ImU32 flatColor,
                        const ShadingContext& sctx,
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
