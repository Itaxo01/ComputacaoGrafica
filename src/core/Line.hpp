#ifndef LINE_H
#define LINE_H

#include "Point.hpp"
#include "imgui.h"

namespace core {
    // Geometry segment used by clipping algorithms and background renderer.
    struct Line {
        core::Point a, b;
        ImU32 color = 0xFF;

        Line() = default;
        Line(core::Point a, core::Point b) : a(a), b(b) {}
    };
}

#endif // LINE_H
