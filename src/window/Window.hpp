#ifndef WINDOW_H
#define WINDOW_H

#include "imgui.h"
#include "Point.hpp"

class Viewport; // Forward declaration

class Window {
private:
    Viewport &viewport;
    core::Point p0 = core::Point(-10, -10); //xmin, ymin (bottom-left)
    core::Point p1 = core::Point(10, 10); //xmax, ymax (top-right)
    float x_offset;
    float y_offset;

    float zoom_factor_acc = 1.0f;
public:
    Window(Viewport &vp): viewport(vp) {}

    void setWorldBounds(const core::Point &bottomLeft, const core::Point &topRight);

    void AddOffset(const float x, const float y) {x_offset += x; y_offset += y;}
    void moveWindow(const float dx, const float dy, const ImVec2 &canvas_sz);
    
    float GetZoomFactor(){return zoom_factor_acc;}
    core::Point GetWorldMin(){return p0;}
    core::Point GetWorldMax(){return p1;}
    core::Point WorldToViewport(const core::Point &wp);
    core::Point ViewportToWorld(const ImVec2 &vp);

    void zoom(const float zoom_factor, const ImVec2 &mouse_pos);
};

#endif