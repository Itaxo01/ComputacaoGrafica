#include "RasterizationEngine.hpp"
#include <algorithm>
#include <cmath>

// Twice the signed area of triangle (a, b, p) — the edge function. Positive on
// one side of edge a→b, negative on the other.
static inline float edge(const ImVec2& a, const ImVec2& b, float px, float py) {
    return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
}

void DrawTriangleFilled(Framebuffer& fb, const ImVec2& a, const ImVec2& b,
                        const ImVec2& c, float za, float zb, float zc,
                        int bb_min_x, int bb_min_y, int bb_max_x, int bb_max_y,
                        const core::Point P[3], const core::Point N[3],
                        const ShadeMaterial& mat, ImU32 flatColor,
                        const ShadingContext& sctx,
                        bool depth_test, bool depth_less, int y_lo, int y_hi) {
    // The bbox arrives precomputed (BuildSortedTriangles), so all that is left
    // here is clamping it to the framebuffer and to the caller's band.
    int minx = std::max(bb_min_x, 0);
    int maxx = std::min(bb_max_x, fb.Width() - 1);
    int miny = std::max(bb_min_y, y_lo);
    int maxy = std::min(bb_max_y, y_hi - 1);
    if (minx > maxx || miny > maxy) return;

    // Degenerate (zero-area) triangle: nothing to fill.
    float area = edge(a, b, c.x, c.y);
    if (area == 0.0f) return;
    // w0+w1+w2 == area, so dividing the weighted depth sum by area normalizes the
    // barycentric coordinates. Screen-space linear interp of post-divide z is exact.
    const float invArea = 1.0f / area;

    // Per-triangle shading precompute (mode 0 = none, 1 = flat, 2 = gouraud, 3 = phong).
    const int mode = sctx.mode;
    ImU32 flatShaded = flatColor;
    core::Color3 gc0, gc1, gc2; // Gouraud per-vertex colors
    if (mode == 1) { // FLAT — one color from the face normal at the centroid
        core::Point fn = cross(P[1] - P[0], P[2] - P[0]);
        core::Point centroid = (P[0] + P[1] + P[2]) * (1.0f / 3.0f);
        flatShaded = packColor(shadePhong(centroid, fn, mat, sctx));
    } else if (mode == 2) { // GOURAUD — light the 3 vertices, interpolate color
        gc0 = shadePhong(P[0], N[0], mat, sctx);
        gc1 = shadePhong(P[1], N[1], mat, sctx);
        gc2 = shadePhong(P[2], N[2], mat, sctx);
    }

    for (int y = miny; y <= maxy; ++y) {
        float py = (float)y + 0.5f;
        for (int x = minx; x <= maxx; ++x) {
            float px = (float)x + 0.5f;
            float w0 = edge(b, c, px, py);
            float w1 = edge(c, a, px, py);
            float w2 = edge(a, b, px, py);
            // Inside when all edge functions share a sign (works for either winding).
            bool inside = (w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                          (w0 <= 0 && w1 <= 0 && w2 <= 0);
            if (!inside) continue;

            ImU32 col;
            if (mode == 0) {
                col = flatColor;
            } else if (mode == 1) {
                col = flatShaded;
            } else {
                float l0 = w0 * invArea, l1 = w1 * invArea, l2 = w2 * invArea;
                if (mode == 2) { // Gouraud: interpolate the vertex colors
                    col = packColor({ l0*gc0.r + l1*gc1.r + l2*gc2.r,
                                      l0*gc0.g + l1*gc1.g + l2*gc2.g,
                                      l0*gc0.b + l1*gc1.b + l2*gc2.b });
                } else {         // Phong: interpolate position + normal, light per pixel
                    core::Point Pp = P[0]*l0 + P[1]*l1 + P[2]*l2;
                    core::Point Np = N[0]*l0 + N[1]*l1 + N[2]*l2;
                    col = packColor(shadePhong(Pp, Np, mat, sctx));
                }
            }

            if (depth_test) {
                float z = (w0 * za + w1 * zb + w2 * zc) * invArea;
                fb.SetPixelDepthUnchecked(x, y, col, z, depth_less);
            } else {
                fb.SetPixelUnchecked(x, y, col);
            }
        }
    }
}

void DrawLine(Framebuffer& fb, float x0f, float y0f, float z0, float x1f, float y1f, float z1,
              ImU32 color, int thickness, bool depth_test, bool depth_less, int y_lo, int y_hi) {
    int x0 = (int)std::lround(x0f), y0 = (int)std::lround(y0f);
    int x1 = (int)std::lround(x1f), y1 = (int)std::lround(y1f);

    // Parameterize depth along the line's dominant axis (0 at start, 1 at end).
    const int sx0 = x0, sy0 = y0;
    const bool xmajor = std::abs(x1 - sx0) >= std::abs(y1 - sy0);
    const int span = xmajor ? (x1 - sx0) : (y1 - sy0);
    const float invSpan = (span != 0) ? 1.0f / (float)span : 0.0f;

    int dx =  std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    if (thickness < 1) thickness = 1;

    while (true) {
        float z = z0;
        if (depth_test) {
            float t = (float)((xmajor ? (x0 - sx0) : (y0 - sy0))) * invSpan;
            z = z0 + t * (z1 - z0);
        }
        for (int oy = 0; oy < thickness; ++oy) {
            int yy = y0 + oy;
            if (yy < y_lo || yy >= y_hi) continue;
            for (int ox = 0; ox < thickness; ++ox) {
                int xx = x0 + ox;
                if (!depth_test || fb.DepthPasses(xx, yy, z, depth_less))
                    fb.SetPixel(xx, yy, color);
            }
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void DrawPoint(Framebuffer& fb, float x, float y, float z, ImU32 color, int half,
               bool depth_test, bool depth_less, int y_lo, int y_hi) {
    int cx = (int)std::lround(x), cy = (int)std::lround(y);
    for (int oy = -half; oy <= half; ++oy) {
        int yy = cy + oy;
        if (yy < y_lo || yy >= y_hi) continue;
        for (int ox = -half; ox <= half; ++ox) {
            int xx = cx + ox;
            if (!depth_test || fb.DepthPasses(xx, yy, z, depth_less))
                fb.SetPixel(xx, yy, color);
        }
    }
}
