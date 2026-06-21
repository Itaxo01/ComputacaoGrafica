#pragma once
#include <string>
#include "imgui.h"

namespace core {

    // RGB triple with components in [0,1], as stored in Wavefront .mtl files.
    struct Color3 {
        float r = 0.0f, g = 0.0f, b = 0.0f;
    };

    // A material owned by an Object. Models a full Wavefront .mtl entry (newmtl).
    // Only `color` and `filled` currently affect rendering; the remaining fields
    // are parsed and stored so they survive a load/save round-trip, but the
    // renderer has no lighting/texturing model to consume them yet.
    struct Material {
        // ── Effective values used by the renderer ──────────────────────────────
        ImU32 color  = 0xFF;          // RENDERED: flat draw color (derived from `diffuse` + `dissolve` on import)
        bool  filled = false;         // RENDERED: whether polygons are filled

        // ── Standard .mtl scalar / color parameters ─────────────────────────────
        std::string name;             // newmtl <name> — material identifier used by `usemtl`
        Color3 ambient;               // Ka  — ambient reflectivity        (not rendered: no lighting model)
        Color3 diffuse  = {1, 1, 1};  // Kd  — diffuse reflectivity        (rendered: mirrored into `color`)
        Color3 specular;              // Ks  — specular reflectivity       (not rendered: no lighting model)
        Color3 emissive;              // Ke  — emissive color              (not rendered: no lighting model)
        Color3 transmission = {1,1,1};// Tf  — transmission filter         (not rendered: no transparency model)
        float  shininess = 0.0f;      // Ns  — specular exponent           (not rendered: no lighting model)
        float  optical_density = 1.0f;// Ni  — index of refraction         (not rendered: no refraction)
        float  dissolve = 1.0f;       // d   — opacity, 1=opaque 0=clear   (partially rendered: maps to color alpha)
        int    illum = 2;             // illum — illumination model id     (not rendered: no lighting model)

        // ── Standard .mtl texture maps (paths) ──────────────────────────────────
        std::string map_Ka;           // map_Ka   — ambient texture        (not rendered: no texturing)
        std::string map_Kd;           // map_Kd   — diffuse texture        (not rendered: no texturing)
        std::string map_Ks;           // map_Ks   — specular texture       (not rendered: no texturing)
        std::string map_Ns;           // map_Ns   — shininess texture      (not rendered: no texturing)
        std::string map_d;            // map_d    — opacity texture        (not rendered: no texturing)
        std::string map_bump;         // map_Bump / bump — normal/bump map (not rendered: no texturing)
    };
}
