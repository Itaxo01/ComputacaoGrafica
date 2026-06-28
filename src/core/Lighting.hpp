#pragma once
#include <vector>
#include "Light.hpp"
#include "Material.hpp" // core::Color3

// Global lighting / shading configuration. Kept separate from AppConfig because
// lights hold core::Point, and Point.hpp includes AppConfig.hpp (would cycle).
namespace Lighting {
    enum Mode { NONE = 0, FLAT = 1, GOURAUD = 2, PHONG = 3 };

    extern int mode;                    // Lighting::Mode (NONE keeps the old flat-color look)
    extern core::Color3 ambient;        // global ambient intensity
    extern std::vector<core::Light> lights;
    extern bool headlight;              // add a light co-located with the camera eye each frame
    extern core::Color3 headlight_color;
    extern float headlight_intensity;
}
