#include "RasterizationEngine.hpp"
#include <algorithm>
#include <cmath>

// Twice the signed area of triangle (a, b, p) — the edge function. Positive on
// one side of edge a→b, negative on the other.
static inline float edge(const ImVec2& a, const ImVec2& b, float px, float py) {
    return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
}

void DrawTriangleFilled(Framebuffer& fb, const ImVec2& a, const ImVec2& b,
                        const ImVec2& c, ImU32 color, int y_lo, int y_hi) {
    int minx = (int)std::floor(std::min({a.x, b.x, c.x}));
    int maxx = (int)std::ceil (std::max({a.x, b.x, c.x}));
    int miny = (int)std::floor(std::min({a.y, b.y, c.y}));
    int maxy = (int)std::ceil (std::max({a.y, b.y, c.y}));

    minx = std::max(minx, 0);
    maxx = std::min(maxx, fb.Width() - 1);
    miny = std::max(miny, y_lo);
    maxy = std::min(maxy, y_hi - 1);
    if (minx > maxx || miny > maxy) return;

    // Degenerate (zero-area) triangle: nothing to fill.
    float area = edge(a, b, c.x, c.y);
    if (area == 0.0f) return;

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
            if (inside) fb.SetPixel(x, y, color);
        }
    }
}

void DrawLine(Framebuffer& fb, float x0f, float y0f, float x1f, float y1f,
              ImU32 color, int thickness, int y_lo, int y_hi) {
    int x0 = (int)std::lround(x0f), y0 = (int)std::lround(y0f);
    int x1 = (int)std::lround(x1f), y1 = (int)std::lround(y1f);

    int dx =  std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    if (thickness < 1) thickness = 1;

    while (true) {
        for (int oy = 0; oy < thickness; ++oy) {
            int yy = y0 + oy;
            if (yy < y_lo || yy >= y_hi) continue;
            for (int ox = 0; ox < thickness; ++ox)
                fb.SetPixel(x0 + ox, yy, color);
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void DrawPoint(Framebuffer& fb, float x, float y, ImU32 color, int half,
               int y_lo, int y_hi) {
    int cx = (int)std::lround(x), cy = (int)std::lround(y);
    for (int oy = -half; oy <= half; ++oy) {
        int yy = cy + oy;
        if (yy < y_lo || yy >= y_hi) continue;
        for (int ox = -half; ox <= half; ++ox)
            fb.SetPixel(cx + ox, yy, color);
    }
}
