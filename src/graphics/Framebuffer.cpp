#include "Framebuffer.hpp"
#include "ParallelUtils.hpp"
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h> // drags in the system OpenGL headers (glGenTextures, glTexImage2D, ...)
#include <algorithm>

// Windows ships an OpenGL 1.1 <GL/gl.h>, which predates GL_CLAMP_TO_EDGE (added in
// OpenGL 1.2). Its value is fixed by the spec, so define it when the system header
// doesn't. (Linux/Mesa headers already provide it, so this is a no-op there.)
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

void Framebuffer::ensureTexture() {
    if (tex != 0) return;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // 1:1 blit, so filtering barely matters; LINEAR stays clean under minor DPI scaling.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Framebuffer::Resize(int display_w, int display_h, int f) {
    f = std::max(1, f);
    int rw = std::max(1, display_w);
    int rh = std::max(1, display_h);
    int w = rw * f;
    int h = rh * f;
    if (w == width && h == height) return;
    factor = f;
    width = w;   height = h;
    rwidth = rw; rheight = rh;
    pixels.assign((size_t)width * height, 0u);
    depth.assign((size_t)width * height, 0.0f); // cleared per-band each frame when in use
    resolved.assign((size_t)rwidth * rheight, 0u);
    needs_realloc = true; // resolved dimensions changed: re-spec texture storage on next upload
}

void Framebuffer::Clear(ImU32 color) {
    std::fill(pixels.begin(), pixels.end(), color);
}

void Framebuffer::ClearRows(int y_lo, int y_hi, ImU32 color) {
    y_lo = std::max(y_lo, 0);
    y_hi = std::min(y_hi, height);
    if (y_lo >= y_hi) return;
    std::fill(pixels.begin() + (size_t)y_lo * width,
              pixels.begin() + (size_t)y_hi * width, color);
}

void Framebuffer::ClearDepthRows(int y_lo, int y_hi, float far) {
    y_lo = std::max(y_lo, 0);
    y_hi = std::min(y_hi, height);
    if (y_lo >= y_hi) return;
    std::fill(depth.begin() + (size_t)y_lo * width,
              depth.begin() + (size_t)y_hi * width, far);
}

void Framebuffer::Resolve() {
    if (resolved.empty()) return;

    // Fast path: nothing to average at 1x — the scene buffer *is* the output.
    if (factor == 1) {
        resolved = pixels;
        return;
    }

    const int f = factor;
    const float invN = 1.0f / (float)(f * f);
    cg_parallel_chunks((std::size_t)rheight, [&](std::size_t ry0, std::size_t ry1) {
        for (int ry = (int)ry0; ry < (int)ry1; ++ry) {
            for (int rx = 0; rx < rwidth; ++rx) {
                // Accumulate the f x f scene block in premultiplied alpha so the
                // coverage (alpha) weights the color correctly.
                unsigned sumA = 0, sumR = 0, sumG = 0, sumB = 0;
                int bx = rx * f, by = ry * f;
                for (int dy = 0; dy < f; ++dy) {
                    const ImU32* srow = &pixels[(size_t)(by + dy) * width + bx];
                    for (int dx = 0; dx < f; ++dx) {
                        ImU32 p = srow[dx];
                        unsigned a = (p >> IM_COL32_A_SHIFT) & 0xFFu;
                        sumA += a;
                        sumR += ((p >> IM_COL32_R_SHIFT) & 0xFFu) * a;
                        sumG += ((p >> IM_COL32_G_SHIFT) & 0xFFu) * a;
                        sumB += ((p >> IM_COL32_B_SHIFT) & 0xFFu) * a;
                    }
                }
                unsigned outA = (unsigned)(sumA * invN + 0.5f); // average coverage
                unsigned oR = 0, oG = 0, oB = 0;
                if (sumA > 0) { // un-premultiply: weighted-average color of covered samples
                    oR = (unsigned)(sumR / (float)sumA + 0.5f);
                    oG = (unsigned)(sumG / (float)sumA + 0.5f);
                    oB = (unsigned)(sumB / (float)sumA + 0.5f);
                }
                resolved[(size_t)ry * rwidth + rx] = IM_COL32(oR, oG, oB, outA);
            }
        }
    });
}

void Framebuffer::Present(ImDrawList* dl, const ImVec2& p0, const ImVec2& p1) {
    if (rwidth <= 0 || rheight <= 0) return;
    ensureTexture();
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // rows are rwidth*4 bytes, always 4-aligned
    if (needs_realloc) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rwidth, rheight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, resolved.data());
        needs_realloc = false;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rwidth, rheight,
                        GL_RGBA, GL_UNSIGNED_BYTE, resolved.data());
    }
    // ImGui uses a top-left UV origin for images, matching our row 0 = top buffer.
    dl->AddImage((ImTextureID)(intptr_t)tex, p0, p1);
}

Framebuffer::~Framebuffer() {
    if (tex != 0) glDeleteTextures(1, &tex);
}
