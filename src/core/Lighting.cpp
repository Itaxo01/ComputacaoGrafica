#include "Lighting.hpp"

namespace Lighting {
    int mode = NONE;                       // off by default: preserves the flat-color render
    core::Color3 ambient = {0.15f, 0.15f, 0.15f};
    std::vector<core::Light> lights;       // user-placed lights (headlight covers the default)
    bool headlight = true;
    core::Color3 headlight_color = {1.0f, 1.0f, 1.0f};
    float headlight_intensity = 1.0f;
}
