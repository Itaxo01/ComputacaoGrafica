#include "RasterizationEngine.hpp"
#include <algorithm>
#include <cmath>

void DrawSortedTri(Framebuffer& fb, const SortedTri& t, const ShadingContext& sctx,
                   bool depth_test, bool depth_less, int y_lo, int y_hi) {
    // The per-pixel coverage + shading is the shared body; only the pixel commit is
    // CPU-specific (Framebuffer write, depth-tested or overwrite).
    const core::Light* lp = (sctx.lights && !sctx.lights->empty()) ? sctx.lights->data() : nullptr;
    const int nl = sctx.lights ? (int)sctx.lights->size() : 0;
    rasterizeTriangleInto(t, fb.Width(), y_lo, y_hi, sctx.mode, sctx.eye, sctx.ambient, lp, nl,
        [&](int x, int y, float z, ImU32 col) {
            if (depth_test) fb.SetPixelDepthUnchecked(x, y, col, z, depth_less);
            else            fb.SetPixelUnchecked(x, y, col);
        });
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
