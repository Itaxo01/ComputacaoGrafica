#pragma once
#include <vector>
#include "imgui.h"

namespace core {
    std::vector<int> triangulate(std::vector<ImVec2> poly);
}
