#include "Window.hpp"
#include "Transformations.hpp"
#include "Viewport.hpp"
#include "AppConfig.hpp"

Window::Window(Viewport &vp) : viewport(vp), center(0.0f, 0.0f, 0.0f), width(20.0f), height(20.0f), angle(0.0f) {
    camera.view_width  = width;
    camera.view_height = height;
    UpdateNCSMatrix();
}

void Window::setWindowBounds(const core::Point &bottomLeft, const core::Point &topRight) {
    center.x = (bottomLeft.x + topRight.x) / 2.0f;
    center.y = (bottomLeft.y + topRight.y) / 2.0f;
    width = topRight.x - bottomLeft.x;
    height = topRight.y - bottomLeft.y;
    angle = 0.0f;
    UpdateNCSMatrix();
}

/*
    2D: centraliza, rotaciona e escala para [-1,1].
    3D ortográfico: aplica VRC (câmera) e escala para [-1,1] (projeção paralela ortogonal).
    3D perspectivo: aplica VRC, depois a matriz de perspectiva com divisão por w, depois escala.
    Chamada internamente toda vez que a window for atualizada.
*/
void Window::UpdateNCSMatrix(){
    if (AppConfig::is3d) {
        if (AppConfig::perspective) {
            core::mat4 scale    = core::getScalingMatrix(2.0f / camera.view_width,
                                                         2.0f / camera.view_height);
            core::mat4 persp    = camera.GetPerspectiveMatrix();
            this->NCSTransformMatrix        = scale * persp * camera.GetVRCMatrix();

            // Inverse (screen → world): resolves a click onto the view plane
            // (z=0 in VRC), where the perspective projection is the identity in
            // x/y. So the inverse is just VRC^-1 * Scale^-1 — no perspective term
            // (adding one would scale pan/picking deltas by ~1/focal_distance).
            core::mat4 inv_scale = core::getScalingMatrix(camera.view_width  / 2.0f,
                                                          camera.view_height / 2.0f, 1.0f);
            this->InverseNCSTransformMatrix = camera.GetInverseVRCMatrix() * inv_scale;
        } else {
            core::mat4 scale = core::getScalingMatrix(2.0f / camera.view_width,
                                                      2.0f / camera.view_height);
            this->NCSTransformMatrix = scale * camera.GetVRCMatrix();

            core::mat4 inv_scale = core::getScalingMatrix(camera.view_width  / 2.0f,
                                                          camera.view_height / 2.0f, 1.0f);
            this->InverseNCSTransformMatrix = camera.GetInverseVRCMatrix() * inv_scale;
        }
    } else {
        this->NCSTransformMatrix = \
                            core::getScalingMatrix(2.0f / width, 2.0f / height) *\
                            core::getRotationMatrixZ(-angle) * \
                            core::getTranslationMatrix(-center.x, -center.y);

        this->InverseNCSTransformMatrix = \
                            core::getTranslationMatrix(center.x, center.y, 0.0f) *\
                            core::getRotationMatrixZ(angle) * \
                            core::getScalingMatrix(width / 2.0f, height / 2.0f, 1.0f);
    }
}

WindowAttributes Window::getWindowAttributes() const {
    WindowAttributes w;
    if (AppConfig::is3d) {
        w.center = camera.vrp;
        w.width  = camera.view_width;
        w.height = camera.view_height;
        w.angle  = 0.0f;
        w.vpn    = camera.vpn;
        w.focal_distance = camera.focal_distance;
        w.perspective    = AppConfig::perspective;
    } else {
        w.center = center;
        w.width  = width;
        w.height = height;
        w.angle  = angle;
    }
    return w;
}

core::mat4 Window::GetProjectionScaleMatrix() const {
    // Scale(2/vw, 2/vh) * Perspective. Applied to VRC-space points (after the
    // near-plane clip), with the perspective divide happening in mat4 * Point.
    core::mat4 scale = core::getScalingMatrix(2.0f / camera.view_width,
                                              2.0f / camera.view_height);
    return scale * camera.GetPerspectiveMatrix();
}

void Window::adjustPerspective(float factor) {
    camera.adjustFocalDistance(factor);
    UpdateNCSMatrix();
}

core::Point Window::NCSToViewport(const core::Point &p) const{
    ImVec2 canvas_size = viewport.GetCanvasSize();

    // x vai de [-1, 1] para [0, canvas_size.x];
    float screen_x = ((p.x+1.0f) / 2.0f) * canvas_size.x;
    // y vai de [-1, 1] para [canvas_size.y, 0];
    float screen_y = (1.0f - ((p.y + 1.0f)/2.0f)) * canvas_size.y;
    return core::Point(screen_x, screen_y, 0.0f);
}

core::Point Window::ViewportToNCS(const core::Point &p) const {
    ImVec2 canvas_size = viewport.GetCanvasSize();
    float ncs_x = (p.x/canvas_size.x) * 2.0f - 1.0f;
    float ncs_y = 1.0f - (p.y/canvas_size.y) * 2.0f;
    
    return core::Point(ncs_x, ncs_y);
}

core::Point Window::WindowToViewport(const core::Point &wp) const {
    core::Point ncs_p = NCSTransformMatrix * wp;
    core::Point screen_p = NCSToViewport(ncs_p);

    auto cp = viewport.GetCanvasP();
    screen_p.x += cp.first.x;
    screen_p.y += cp.first.y;
    return screen_p;
}

core::Point Window::ViewportToWindow(const ImVec2 &vp) const {
    auto cp = viewport.GetCanvasP();
    core::Point screen_p;
    screen_p.x = vp.x - cp.first.x;
    screen_p.y = vp.y - cp.first.y;
    core::Point ncs_p = ViewportToNCS(screen_p);

    // Matriz inversa da NCS
    return InverseNCSTransformMatrix * ncs_p;
}


void Window::zoom(const float zoom_factor, const ImVec2 &mouse_pos){
    if (AppConfig::is3d) {
        // Anchored zoom, 3D analog of the 2D path below: keep the world point under
        // the mouse (resolved onto the view plane) fixed on screen. ViewportToWindow
        // works in both ortho and perspective, so shifting the camera VRP by the
        // pre/post-zoom anchor difference re-anchors the view.
        core::Point old_anchor = ViewportToWindow(mouse_pos);
        camera.zoom(zoom_factor);
        UpdateNCSMatrix();
        core::Point new_anchor = ViewportToWindow(mouse_pos);
        camera.vrp += (old_anchor - new_anchor);
        UpdateNCSMatrix();
        return;
    }

    core::Point old_window_anchor = ViewportToWindow(mouse_pos);

    width *= zoom_factor;
    height *= zoom_factor;
    zoom_factor_acc *= zoom_factor;

    UpdateNCSMatrix();

    core::Point new_window_anchor = ViewportToWindow(mouse_pos);
    center.x += (old_window_anchor.x - new_window_anchor.x);
    center.y += (old_window_anchor.y - new_window_anchor.y);

    UpdateNCSMatrix();
}

void Window::moveWindow(const float dx, const float dy, const ImVec2 &canvas_sz){
    if (AppConfig::is3d) {
        // Pan in the camera's view plane: p2 - p1 is the world displacement of the
        // dragged point on the view plane (a combination of the view's right/up
        // axes), so applying all three components keeps the scene following the
        // cursor at any orientation. Dropping Y would freeze vertical panning
        // whenever the view looks along the XZ plane (v ≈ world Y).
        core::Point p1 = ViewportToWindow(ImVec2(0.0f, 0.0f));
        core::Point p2 = ViewportToWindow(ImVec2(dx, dy));
        camera.vrp.x -= (p2.x - p1.x);
        camera.vrp.y -= (p2.y - p1.y);
        camera.vrp.z -= (p2.z - p1.z);
        UpdateNCSMatrix();
        return;
    }

    core::Point p1 = ViewportToWindow(ImVec2(0, 0));
    core::Point p2 = ViewportToWindow(ImVec2(dx, dy));
    center.x -= (p2.x - p1.x);
    center.y -= (p2.y - p1.y);
    UpdateNCSMatrix();
}

void Window::rotate(float degrees) {
    if (AppConfig::is3d) {
        camera.orbit(degrees, 0.0f);
    } else {
        angle += degrees;
    }
    UpdateNCSMatrix();
}

void Window::orbitPitch(float degrees) {
    camera.orbit(0.0f, degrees);
    UpdateNCSMatrix();
}

float Window::getBoundingBoxHalfSize() const {
    return camera.view_width * 0.31f;
}

std::pair<core::Point, core::Point> Window::getClipBoundsNCS() const {
    // 2D, or 3D with the bounding box hidden: clip to the full viewport [-1,1].
    // The bounding-box-derived clip region is only used while the box is visible.
    if (!AppConfig::is3d || !AppConfig::show_bounding_box)
        return { core::Point(-1.0f, -1.0f, 0.0f), core::Point(1.0f, 1.0f, 0.0f) };

    float B  = getBoundingBoxHalfSize();
    float cx = camera.vrp.x, cy = camera.vrp.y, cz = camera.vrp.z;

    const core::Point corners[8] = {
        {cx-B, cy-B, cz-B}, {cx+B, cy-B, cz-B},
        {cx-B, cy+B, cz-B}, {cx+B, cy+B, cz-B},
        {cx-B, cy-B, cz+B}, {cx+B, cy-B, cz+B},
        {cx-B, cy+B, cz+B}, {cx+B, cy+B, cz+B},
    };

    float x_min =  1e9f, x_max = -1e9f;
    float y_min =  1e9f, y_max = -1e9f;
    for (const auto& c : corners) {
        core::Point p = NCSTransformMatrix * c;
        if (p.x < x_min) x_min = p.x;
        if (p.x > x_max) x_max = p.x;
        if (p.y < y_min) y_min = p.y;
        if (p.y > y_max) y_max = p.y;
    }

    // Clamp to viewport so clip region never exceeds screen
    if (x_min < -1.0f) x_min = -1.0f;
    if (x_max >  1.0f) x_max =  1.0f;
    if (y_min < -1.0f) y_min = -1.0f;
    if (y_max >  1.0f) y_max =  1.0f;

    return { core::Point(x_min, y_min, 0.0f), core::Point(x_max, y_max, 0.0f) };
}

void Window::OnModeChanged() {
    if (AppConfig::is3d) {
        // Y-up convention: vpn = normalize(1,1,1) gives standard isometric view.
        // Result: X goes right, Y goes up on screen, Z goes left-down (depth).
        camera.vrp           = {0.0f, 0.0f, 0.0f};
        camera.vpn           = {0.5774f, 0.5774f, 0.5774f};
        camera.vup           = {0.0f, 1.0f, 0.0f};
        camera.view_width    = 20.0f;
        camera.view_height   = 20.0f;
        camera.focal_distance = 1000.0f; // large d ≈ orthographic; Shift+scroll for perspective
    }
    UpdateNCSMatrix();
}