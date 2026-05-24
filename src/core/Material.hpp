#pragma once
#include "imgui.h"

namespace core {
    struct Material {
        ImU32 color = 0xFF;
        bool filled = false;
    };
}
