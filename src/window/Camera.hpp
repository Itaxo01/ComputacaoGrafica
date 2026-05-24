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
class Camera {
public:
    core::Point vrp{0.0f, 0.0f, 0.0f}; // View Reference Point (camera origin)
    core::Point vpn{0.5774f, 0.5774f, 0.5774f}; // View Plane Normal (becomes +Z in VRC)
    core::Point vup{0.0f, 1.0f, 0.0f};          // View Up vector (Y-up convention)

    float view_width  = 20.0f;
    float view_height = 20.0f;

    Camera() = default;

    // Returns R * T(-VRP): transforms world → VRC space where VPN becomes Z.
    core::mat4 GetVRCMatrix() const;

    // Returns the inverse of GetVRCMatrix(): VRC space → world.
    core::mat4 GetInverseVRCMatrix() const;

    // Translate VRP along view-space u (right) and v (up) directions.
    void pan(float du, float dv);

    // Scale view volume (zoom in/out for orthographic).
    void zoom(float factor);

    // Orbit: rotate VPN around world Y axis (yaw, Y-up) and local u axis (pitch).
    void orbit(float dyaw_deg, float dpitch_deg);

    struct Basis { core::Point u, v, n; };
    inline Basis computeBasis() const;
};
