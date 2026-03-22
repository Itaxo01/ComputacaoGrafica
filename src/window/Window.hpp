#ifndef WINDOW_H
#define WINDOW_H

#include "imgui.h"
#include "Point.hpp"

class Viewport; // Forward declaration

class Window {
private:
    Viewport &viewport;
    core::Point p0 = core::Point(0, 0); //xmin, ymin (bottom-left)
    core::Point p1 = core::Point(1000, 1000); //xmax, ymax (top-right)
    float x_offset;
    float y_offset;
public:
    Window(Viewport &vp): viewport(vp) {}

    void setWorldBounds(core::Point &bottomLeft, core::Point &topRight);

    void AddOffset(float x, float y) {x_offset += x; y_offset += y;}
    core::Point WorldToViewport(core::Point wp);
    core::Point ViewportToWorld(ImVec2 vp);
};

#endif