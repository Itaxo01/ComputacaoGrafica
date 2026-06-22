#pragma once
#include "imgui.h"
#include <vector>

// CPU framebuffer: the scene is rasterized into a plain RGBA pixel array on the
// CPU (no OpenGL drawing) and only *presented* by uploading it to a GL texture
// that draw_list->AddImage() blits over the viewport.
//
// Anti-aliasing is supersampling (SSAA): the scene buffer is `factor` x the
// viewport resolution, then Resolve() box-downsamples it into a display-size
// buffer on the CPU. The downsample averages in PREMULTIPLIED alpha (coverage-
// weighted), which is the correct way to filter against the transparent
// background — straight RGBA averaging would darken edges. The resolved buffer
// is what gets uploaded, so the result is independent of GPU sampler behavior,
// DPI scaling and rect alignment (all of which made the old "let GL_LINEAR
// minify it" trick unreliable).
class Framebuffer {
public:
    // (Re)allocate buffers. `display_w/h` is the viewport size in pixels; the
    // scene buffer is allocated at `factor` x that (factor clamped to >= 1).
    // No-op when nothing changed.
    void Resize(int display_w, int display_h, int factor);

    // Reset every scene pixel to `color` (default: fully transparent, so the grid
    // / axes drawn underneath the blit show through). The hot path uses ClearRows
    // per band instead, so the wipe is parallelized and cache-local with the fill.
    void Clear(ImU32 color = 0u);

    // Clear scene rows [y_lo, y_hi) only — used by the banded parallel rasterizer.
    void ClearRows(int y_lo, int y_hi, ImU32 color = 0u);

    // Clear depth rows [y_lo, y_hi) to `far` (the farthest value, so the first
    // fragment always wins). Only needed when the z-buffer is in use.
    void ClearDepthRows(int y_lo, int y_hi, float far);

    // Scene (supersampled) dimensions — the space all rasterization happens in.
    inline int Width()  const { return width;  }
    inline int Height() const { return height; }

    // Bounds-checked single-pixel write into the scene buffer (draw_pixel).
    // Overwrite only (no depth/alpha blending yet — that arrives with the
    // z-buffer step). Color is ImU32 in ImGui's native R,G,B,A byte order.
    inline void SetPixel(int x, int y, ImU32 color) {
        if ((unsigned)x < (unsigned)width && (unsigned)y < (unsigned)height)
            pixels[(size_t)y * width + x] = color;
    }

    // Depth-tested write: keep the fragment only if it is nearer than what is
    // stored, and on success update BOTH color and depth. `less` selects the
    // nearer direction (true: nearer = smaller z). Used by filled triangles.
    inline bool SetPixelDepth(int x, int y, ImU32 color, float z, bool less) {
        if ((unsigned)x >= (unsigned)width || (unsigned)y >= (unsigned)height) return false;
        size_t i = (size_t)y * width + x;
        if (less ? (z < depth[i]) : (z > depth[i])) {
            depth[i] = z;
            pixels[i] = color;
            return true;
        }
        return false;
    }

    // Unchecked writes — NO bounds check. Only call from a loop whose iteration
    // range is already clamped to the framebuffer (the triangle rasterizer clamps
    // its bbox), so the per-pixel branch is removed from the hot fill path.
    inline void SetPixelUnchecked(int x, int y, ImU32 color) {
        pixels[(size_t)y * width + x] = color;
    }
    inline bool SetPixelDepthUnchecked(int x, int y, ImU32 color, float z, bool less) {
        size_t i = (size_t)y * width + x;
        if (less ? (z < depth[i]) : (z > depth[i])) {
            depth[i] = z;
            pixels[i] = color;
            return true;
        }
        return false;
    }

    // Read-only depth test (nearer-or-equal), without writing depth. Used by
    // wireframe lines / points so they are hidden behind solids but do not
    // pollute the depth buffer (and coplanar edges-on-their-own-surface still
    // show, since the test is inclusive). Out-of-bounds counts as failing.
    inline bool DepthPasses(int x, int y, float z, bool less) const {
        if ((unsigned)x >= (unsigned)width || (unsigned)y >= (unsigned)height) return false;
        size_t i = (size_t)y * width + x;
        return less ? (z <= depth[i]) : (z >= depth[i]);
    }

    // Box-downsample the scene buffer into the display-size resolve buffer
    // (premultiplied average). Run once per frame after rasterizing, before
    // Present. Parallelized over output rows.
    void Resolve();

    // Upload the resolved buffer to the GL texture and record an AddImage over
    // the display-space rectangle [p0, p1].
    void Present(ImDrawList* dl, const ImVec2& p0, const ImVec2& p1);

    ~Framebuffer();

private:
    int width = 0, height = 0;      // scene (supersampled) dimensions
    int rwidth = 0, rheight = 0;    // resolved (display) dimensions
    int factor = 1;                 // supersample factor: width == rwidth * factor
    std::vector<ImU32> pixels;      // scene buffer (rasterized here)
    std::vector<float> depth;       // per-pixel depth, same size as pixels (z-buffer)
    std::vector<ImU32> resolved;    // display-size buffer (uploaded)
    unsigned int tex = 0;           // GLuint, kept opaque to avoid a GL include here
    bool needs_realloc = false;     // true after a resize: next upload must reallocate

    void ensureTexture();
};
