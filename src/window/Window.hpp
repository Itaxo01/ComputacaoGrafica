#ifndef WINDOW_H
#define WINDOW_H

#include "Transformations.hpp"
#include "imgui.h"
#include "Point.hpp"
#include "Mat4.hpp"
#include "Camera.hpp"

class Viewport; // Forward declaration

struct WindowAttributes {
    core::Point center;
    float width, height;
    float angle;
    core::Point vpn{0.0f, 0.0f, 1.0f}; // tracks camera orientation for cache invalidation in 3D
    float focal_distance = 20.0f;       // tracks perspective COP distance
    WindowAttributes(){}
    WindowAttributes(const core::Point& center, float w, float h, float a): center(center), width(w), height(h), angle(a) {};
    friend bool operator==(const WindowAttributes &a, const WindowAttributes &b){
        return a.center == b.center && a.width == b.width && a.height == b.height && a.angle == b.angle && a.vpn == b.vpn && a.focal_distance == b.focal_distance;
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

    core::mat4 NCSTransformMatrix = core::mat4(true);

    // Inversa é guardada para reverter do viewport para window
    core::mat4 InverseNCSTransformMatrix = core::mat4(true);

    void UpdateNCSMatrix();

public:
    Camera camera; // 3D camera (VRC)

    Window(Viewport &vp);

    void setWindowBounds(const core::Point &p0, const core::Point &p1);
    void moveWindow(const float dx, const float dy, const ImVec2 &canvas_sz);

    WindowAttributes getWindowAttributes() const;

    core::Point GetWindowMin() const {return core::Point(center.x - width/2, center.y - height/2);}
    core::Point GetWindowMax() const {return core::Point(center.x + width/2, center.y + height/2);}

    core::Point WindowToViewport(const core::Point &wp) const;
    core::Point ViewportToWindow(const ImVec2 &vp) const;

    core::mat4 GetWindowNCSMatrix() const {return NCSTransformMatrix;}
    core::mat4 GetWindowInverseNCSMatrix() const {return InverseNCSTransformMatrix;}
    core::Point NCSToViewport(const core::Point &p) const;
    core::Point ViewportToNCS(const core::Point &p) const;

    // 2D: rotate the window plane; 3D: orbit camera yaw.
    void rotate(float degrees);

    // 3D only: orbit camera pitch (look up/down).
    void orbitPitch(float degrees);

    // Call when AppConfig::is3d changes — resets camera to default 3D angle and rebuilds matrix.
    void OnModeChanged();

    void ApplyTransformation(const core::mat4 &m) {
        this->NCSTransformMatrix *= m;
    };

    void zoom(const float zoom_factor, const ImVec2 &mouse_pos);
};

#endif