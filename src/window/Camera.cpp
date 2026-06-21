#include "Camera.hpp"

static inline core::Point normalize3(const core::Point &p) {
    float len = std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
    if (len < 1e-6f) return {0.0f, 0.0f, 1.0f};
    return p / len;
}

inline Camera::Basis Camera::computeBasis() const {
    core::Point n = normalize3(vpn);
    core::Point u_raw = cross(vup, n);
    core::Point u = normalize3(u_raw);
    // Recompute u if vup is parallel to n (degenerate case)
    if (u.x == 0.0f && u.y == 0.0f && u.z == 0.0f) {
        core::Point fallback = (std::abs(n.x) < 0.9f) ? core::Point{1,0,0} : core::Point{0,1,0};
        u = normalize3(cross(fallback, n));
    }
    core::Point v = cross(n, u);
    return {u, v, n};
}

core::mat4 Camera::GetVRCMatrix() const {
    auto [u, v, n] = computeBasis();

    // Rotation rows: [u; v; n] (aligns u→X, v→Y, n→Z)
    core::mat4 R;
    R[0][0] = u.x; R[0][1] = u.y; R[0][2] = u.z; R[0][3] = 0.0f;
    R[1][0] = v.x; R[1][1] = v.y; R[1][2] = v.z; R[1][3] = 0.0f;
    R[2][0] = n.x; R[2][1] = n.y; R[2][2] = n.z; R[2][3] = 0.0f;
    R[3][0] = 0.0f; R[3][1] = 0.0f; R[3][2] = 0.0f; R[3][3] = 1.0f;

    core::mat4 T = core::getTranslationMatrix(-vrp.x, -vrp.y, -vrp.z);
    return R * T;
}

core::mat4 Camera::GetInverseVRCMatrix() const {
    auto [u, v, n] = computeBasis();

    // Inverse rotation = transpose of R
    core::mat4 RT;
    RT[0][0] = u.x; RT[0][1] = v.x; RT[0][2] = n.x; RT[0][3] = 0.0f;
    RT[1][0] = u.y; RT[1][1] = v.y; RT[1][2] = n.y; RT[1][3] = 0.0f;
    RT[2][0] = u.z; RT[2][1] = v.z; RT[2][2] = n.z; RT[2][3] = 0.0f;
    RT[3][0] = 0.0f; RT[3][1] = 0.0f; RT[3][2] = 0.0f; RT[3][3] = 1.0f;

    core::mat4 T = core::getTranslationMatrix(vrp.x, vrp.y, vrp.z);
    return T * RT;
}

void Camera::pan(float du, float dv) {
    auto [u, v, n] = computeBasis();
    vrp.x += u.x * du + v.x * dv;
    vrp.y += u.y * du + v.y * dv;
    vrp.z += u.z * du + v.z * dv;
}

void Camera::zoom(float factor) {
    // Zoom always scales the view volume (works in both orthographic and
    // perspective). factor < 1 = zoom in (view volume shrinks). In perspective
    // the perspective strength is controlled separately via adjustFocalDistance.
    view_width  *= factor;
    view_height *= factor;
}

void Camera::adjustFocalDistance(float factor) {
    // Moves the Centre of Projection along the view axis (changes the COP
    // distance d). Larger d = COP farther back = weaker perspective (telephoto,
    // approaches orthographic); smaller d = stronger perspective (wide angle).
    focal_distance *= (1.0f / factor);
    if (focal_distance < 0.5f) focal_distance = 0.5f; // prevent degenerate COP
}

core::mat4 Camera::GetPerspectiveMatrix() const {
    // COP at VRC (0,0,-d), view plane at z=0. A VRC point (x,y,z,1) maps to
    // (x*d, y*d, z*d, z+d); after the divide-by-w this yields the projected
    // point x' = x*d/(z+d), y' = y*d/(z+d). The +1 in the w-row is what makes
    // w = z + d (distance from the COP), so points farther from the COP shrink.
    float d = focal_distance;
    core::mat4 P;
    P[0][0] = d;    P[0][1] = 0.0f; P[0][2] = 0.0f; P[0][3] = 0.0f;
    P[1][0] = 0.0f; P[1][1] = d;    P[1][2] = 0.0f; P[1][3] = 0.0f;
    P[2][0] = 0.0f; P[2][1] = 0.0f; P[2][2] = d;    P[2][3] = 0.0f;
    P[3][0] = 0.0f; P[3][1] = 0.0f; P[3][2] = 1.0f; P[3][3] = d;
    return P;
}

void Camera::orbit(float dyaw_deg, float dpitch_deg) {
    auto basis = computeBasis();

    if (std::abs(dyaw_deg) > 1e-6f) {
        // Yaw around world Y (Y-up convention used in 3D mode).
        core::mat4 yaw = core::getRotationMatrixY(dyaw_deg);
        vpn = yaw * vpn;
        vup = yaw * vup;
    }
    if (std::abs(dpitch_deg) > 1e-6f) {
        // Rotate VPN and VUP around the local u (right) axis — Rodrigues
        auto [u, v, n] = computeBasis(); // recompute after yaw
        float rad = dpitch_deg * (float)M_PI / 180.0f;
        float c = std::cos(rad), s = std::sin(rad), ic = 1.0f - c;
        float ax = u.x, ay = u.y, az = u.z;

        auto rot_vec = [&](core::Point &p) {
            float nx = p.x*(c + ax*ax*ic) + p.y*(ax*ay*ic - az*s) + p.z*(ax*az*ic + ay*s);
            float ny = p.x*(ay*ax*ic + az*s) + p.y*(c + ay*ay*ic) + p.z*(ay*az*ic - ax*s);
            float nz = p.x*(az*ax*ic - ay*s) + p.y*(az*ay*ic + ax*s) + p.z*(c + az*az*ic);
            p.x = nx; p.y = ny; p.z = nz;
        };
        rot_vec(vpn);
        rot_vec(vup);
    }
}