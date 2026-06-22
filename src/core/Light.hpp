#pragma once
#include "Point.hpp"
#include "Material.hpp" // core::Color3

namespace core {
    // A simple positional (point) light for the Phong illumination model.
    // Positions are in world space; color is the light's RGB intensity in [0,1]
    // (scaled by `intensity`). Diffuse and specular share this color.
    struct Light {
        Point  position{0.0f, 0.0f, 0.0f};
        Color3 color{1.0f, 1.0f, 1.0f};
        float  intensity = 1.0f;
        bool   enabled = true;
    };
}
