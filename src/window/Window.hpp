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

    void setWorldBounds(const core::Point &bottomLeft, const core::Point &topRight);

    void AddOffset(const float x, const float y) {x_offset += x; y_offset += y;}
    void moveWindow(const float x, const float y){
        p0.x += x, p1.x += x;
        p0.y += y, p1.y += y;
    }
    
    core::Point WorldToViewport(const core::Point &wp);
    core::Point ViewportToWorld(const ImVec2 &vp);

    void zoom(const float zoom_factor, const ImVec2 &mouse_pos);
};

#endif