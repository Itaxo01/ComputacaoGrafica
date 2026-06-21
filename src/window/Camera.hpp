#pragma once
#include "Point.hpp"
#include "Mat4.hpp"
#include "Transformations.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// View Reference Coordinate (VRC) camera for orthographic parallel projection.
// After GetVRCMatrix(), the view plane normal VPN aligns with +Z, so the
// orthographic projection is simply dropping the Z coordinate.
// Perspective: the Centre of Projection (COP) sits at (0, 0, -focal_distance)
// in VRC space (i.e. behind the view plane at Z=0).  A point (x,y,z) in VRC
// space projects onto the view plane as:
//      x' = x * d / (z + d),   y' = y * d / (z + d)
// where d = focal_distance. 
class Camera {
public:
    core::Point vrp{0.0f, 0.0f, 0.0f}; // View Reference Point (camera origin)
    core::Point vpn{0.5774f, 0.5774f, 0.5774f}; // View Plane Normal (becomes +Z in VRC)
    core::Point vup{0.0f, 1.0f, 0.0f};          // View Up vector (Y-up convention)

    float view_width  = 20.0f;
    float view_height = 20.0f;

    // Distance from the view plane (Z=0 in VRC) to the Centre of Projection.
    // The COP is at VRC position (0, 0, -focal_distance).
    // Larger values → weaker perspective (approaches orthographic as d → ∞).
    // Default is large so perspective is effectively undistorted on startup;
    // Shift+scroll brings the COP closer for wide-angle distortion.
    float focal_distance = 1000.0f;

    Camera() = default;

    // Returns R * T(-VRP): transforms world → VRC space where VPN becomes +Z.
    core::mat4 GetVRCMatrix() const;

    // Returns the inverse of GetVRCMatrix(): VRC space → world.
    core::mat4 GetInverseVRCMatrix() const;

    // Returns the perspective projection matrix in homogeneous form.
    // After multiplying a VRC-space point (x,y,z,1) by this matrix the result
    // is (x*d, y*d, z*d, z+d).  Divide xyz by w to get the projected point.
    // Only meaningful when AppConfig::perspective is true.
    core::mat4 GetPerspectiveMatrix() const;

    // Translate VRP along view-space u (right) and v (up) directions.
    void pan(float du, float dv);

    // Zoom: scale the view volume (factor < 1 = zoom in). Works in both
    // orthographic and perspective modes.
    void zoom(float factor);

    // Perspective only: move the COP along the view axis by scaling
    // focal_distance. Smaller d = wide angle, larger d = telephoto.
    void adjustFocalDistance(float factor);

    // Orbit: rotate VPN around world Y axis (yaw, Y-up) and local u axis (pitch).
    void orbit(float dyaw_deg, float dpitch_deg);

    struct Basis { core::Point u, v, n; };
    inline Basis computeBasis() const;
};