#ifndef WINDOW_H
#define WINDOW_H

#include "Transformations.hpp"
#include "imgui.h"
#include "Point.hpp"
#include "Matrix.hpp"

class Viewport; // Forward declaration

struct WindowAttributes {
    core::Point center;
    float width, height;
    float angle;
    WindowAttributes(){}
    WindowAttributes(const core::Point& center, float w, float h, float a): center(center), width(w), height(h), angle(a) {};
    friend bool operator==(const WindowAttributes &a, const WindowAttributes &b){
        return a.center == b.center && a.width == b.width && a.height == b.height && a.angle == b.angle;
    }
    friend bool operator!=(const WindowAttributes &a, const WindowAttributes &b){
        return !(a==b);
    }
};

class Window {
private:
    Viewport &viewport;
    core::Point center;
    float width, height; // Mudanças para aplicar o NCS.
    float angle = 0.0f; // window current rotation

    float zoom_factor_acc = 1.0f;

    core::Matrix<float> NCSTransformMatrix = core::Matrix<float>(4, 4, 0.0f, true);

    // Inversa é guardada para reverter do viewport para window
    core::Matrix<float> InverseNCSTransformMatrix = core::Matrix<float>(4, 4, 0.0f, true);

    void UpdateNCSMatrix();

public:
    Window(Viewport &vp);

    void setWindowBounds(const core::Point &p0, const core::Point &p1);
    void moveWindow(const float dx, const float dy, const ImVec2 &canvas_sz);
    
    WindowAttributes getWindowAttributes() const { return WindowAttributes(center, width, height, angle);}

    core::Point GetWindowMin() const {return core::Point(center.x - width/2, center.y - height/2);}
    core::Point GetWindowMax() const {return core::Point(center.x + width/2, center.y + height/2);}

    core::Point WindowToViewport(const core::Point &wp) const;
    core::Point ViewportToWindow(const ImVec2 &vp) const;

    core::Matrix<float> GetWindowNCSMatrix() const {return NCSTransformMatrix;}
    core::Matrix<float> GetWindowInverseNCSMatrix() const {return InverseNCSTransformMatrix;}
    core::Point NCSToViewport(const core::Point &p) const;
    core::Point ViewportToNCS(const core::Point &p) const;

    void rotate(float degrees){
        angle += degrees;
        UpdateNCSMatrix();
    }

    void ApplyTransformation(const core::Matrix<float> &m) {
        this->NCSTransformMatrix *= m;
    };

    void zoom(const float zoom_factor, const ImVec2 &mouse_pos);
};

#endif