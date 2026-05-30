#ifndef WINDOW_H
#define WINDOW_H

#include "Transformations.hpp"
#include "imgui.h"
#include "Point.hpp"
#include "Mat4.hpp"
#include "Camera.hpp"
#include <utility>

class Viewport; // Forward declaration

struct WindowAttributes {
    core::Point center;
    float width, height;
    float angle;
    core::Point vpn{0.0f, 0.0f, 1.0f}; // tracks camera orientation for cache invalidation in 3D
    float focal_distance = 1000.0f;     // tracks perspective COP distance
    bool  perspective = false;          // tracks perspective on/off for cache invalidation
    WindowAttributes(){}
    WindowAttributes(const core::Point& center, float w, float h, float a): center(center), width(w), height(h), angle(a) {};
    friend bool operator==(const WindowAttributes &a, const WindowAttributes &b){
        return a.center == b.center && a.width == b.width && a.height == b.height && a.angle == b.angle && a.vpn == b.vpn && a.focal_distance == b.focal_distance && a.perspective == b.perspective;
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

    // Perspective pipeline stages (3D perspective only). The full transform is
    // split so near-plane clipping can run in VRC space before the divide:
    //   world --GetVRCMatrix--> VRC --(near clip)--> --GetProjectionScaleMatrix--> NCS
    core::mat4 GetVRCMatrix() const { return camera.GetVRCMatrix(); }
    core::mat4 GetProjectionScaleMatrix() const;
    // Z of the near plane in VRC: just in front of the COP (at z=-focal_distance),
    // so every surviving vertex has w = z + d > 0 (safe to divide).
    float GetNearPlaneZ() const { return -camera.focal_distance + 0.1f; }
    core::Point NCSToViewport(const core::Point &p) const;
    core::Point ViewportToNCS(const core::Point &p) const;

    // Half-size of the 3D bounding box (same formula used by RendererBackground).
    float getBoundingBoxHalfSize() const;

    // Returns {clip_min, clip_max} in NCS space.
    // 2D: always [-1,-1] / [1,1].
    // 3D: axis-aligned rectangle enclosing all 8 projected bounding-box corners,
    //     clamped to the viewport bounds.
    std::pair<core::Point, core::Point> getClipBoundsNCS() const;

    // 2D: rotate the window plane; 3D: orbit camera yaw.
    void rotate(float degrees);

    // 3D only: orbit camera pitch (look up/down).
    void orbitPitch(float degrees);

    // Call when AppConfig::is3d changes — resets camera to default 3D angle and rebuilds matrix.
    void OnModeChanged();

    // Call when AppConfig::perspective is toggled — rebuilds the NCS matrix
    // (used by names/picking/clip-bounds) without resetting the camera.
    void OnPerspectiveChanged() { UpdateNCSMatrix(); }

    // Perspective only: move the COP (focal distance) for wide-angle/telephoto.
    void adjustPerspective(float factor);

    void ApplyTransformation(const core::mat4 &m) {
        this->NCSTransformMatrix *= m;
    };

    void zoom(const float zoom_factor, const ImVec2 &mouse_pos);
};

#endif