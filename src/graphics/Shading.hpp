#pragma once
#include "Point.hpp"
#include "Material.hpp" // core::Color3
#include "Light.hpp"
#include "HostDevice.hpp"
#include "imgui.h"
#include <vector>
#include <cmath>

// The subset of a Material the shader needs (extracted once, per object, from the
// full core::Material so the rasterizer doesn't drag strings/maps around).
struct ShadeMaterial {
    core::Color3 ka, kd, ks;
    float shininess = 0.0f;
};

// Per-frame shading inputs handed to the rasterizer. `lights` points at a list the
// Renderer rebuilds each frame (user lights + optional headlight). `mode` is a
// Lighting::Mode (0 = none).
struct ShadingContext {
    int mode = 0;
    core::Point eye;          // world-space eye position (for the view vector V)
    core::Color3 ambient;     // global ambient intensity
    const std::vector<core::Light>* lights = nullptr;
};

// Classic Phong illumination evaluated at one world-space point with one world-space
// normal, over a RAW light array (so it runs on both CPU and GPU — no std::vector).
// Returns linear RGB that may exceed 1 (clamped when packed). Two-sided: the normal
// is flipped to face the viewer so winding/back-faces still light correctly.
CG_HD inline core::Color3 shadePhongRaw(const core::Point& P, core::Point N,
                                        const ShadeMaterial& m, const core::Point& eye,
                                        const core::Color3& ambient,
                                        const core::Light* lights, int nLights) {
    float nl = std::sqrt(dot(N, N));
    if (nl > 1e-12f) N /= nl;

    core::Point V = eye - P;
    float vl = std::sqrt(dot(V, V));
    if (vl > 1e-12f) V /= vl;
    if (dot(N, V) < 0.0f) N = N * -1.0f;

    core::Color3 out{ m.ka.r * ambient.r, m.ka.g * ambient.g, m.ka.b * ambient.b };

    for (int i = 0; i < nLights; ++i) {
        const core::Light& L = lights[i];
        if (!L.enabled) continue;
        core::Point Ld = L.position - P;
        float ll = std::sqrt(dot(Ld, Ld));
        if (ll > 1e-12f) Ld /= ll;

        float ndotl = dot(N, Ld);
        if (ndotl <= 0.0f) continue;          // light behind the surface

        const float ir = L.color.r * L.intensity;
        const float ig = L.color.g * L.intensity;
        const float ib = L.color.b * L.intensity;

        // Diffuse (Lambert)
        out.r += m.kd.r * ir * ndotl;
        out.g += m.kd.g * ig * ndotl;
        out.b += m.kd.b * ib * ndotl;

        // Specular (classic Phong: reflect L about N, raise (R·V) to the shininess)
        if (m.shininess > 0.0f) {
            core::Point R = N * (2.0f * ndotl) - Ld;
            float rv = dot(R, V);
            if (rv > 0.0f) {
                float s = std::pow(rv, m.shininess);
                out.r += m.ks.r * ir * s;
                out.g += m.ks.g * ig * s;
                out.b += m.ks.b * ib * s;
            }
        }
    }
    return out;
}

// Host convenience wrapper: pull the raw light array out of the ShadingContext.
inline core::Color3 shadePhong(const core::Point& P, const core::Point& N,
                               const ShadeMaterial& m, const ShadingContext& ctx) {
    const core::Light* lp = (ctx.lights && !ctx.lights->empty()) ? ctx.lights->data() : nullptr;
    int n = ctx.lights ? (int)ctx.lights->size() : 0;
    return shadePhongRaw(P, N, m, ctx.eye, ctx.ambient, lp, n);
}

// Pack linear RGB (clamped to [0,1]) into ImGui's ImU32, keeping a given alpha.
CG_HD inline ImU32 packColor(const core::Color3& c, int alpha = 255) {
    auto cl = [](float v) { int i = (int)(v * 255.0f + 0.5f); return i < 0 ? 0 : (i > 255 ? 255 : i); };
    return IM_COL32(cl(c.r), cl(c.g), cl(c.b), alpha);
}
