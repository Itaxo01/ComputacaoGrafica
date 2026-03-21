#ifndef WINDOW_H
#define WINDOW_H

#include "viewport.h"
#include "Point.hpp"

class Window {
private:
    //Viewport &viewport = nullptr;
    core::Point p0; //xmin, ymin
    core::Point p1; //xmax, ymax
    float x_offset;
    float y_offset;
public:
    //void SetViewport(Viewport &v) {viewport = v;}
    void AddOffset(float x, float y) {x_offset += x; y_offset += y;}
    core::Point WorldToViewport(core::Point wp);
};

#endif