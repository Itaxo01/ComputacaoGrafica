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
    3D: aplica VRC (câmera) e escala para [-1,1] (projeção paralela ortogonal).
    Chamada internamente toda vez que a window for atualizada.
*/
void Window::UpdateNCSMatrix(){
    if (AppConfig::is3d) {
        // 3D pipeline: Scale(2/vw, 2/vh) * VRC
        core::mat4 scale = core::getScalingMatrix(2.0f / camera.view_width, 2.0f / camera.view_height);
        this->NCSTransformMatrix = scale * camera.GetVRCMatrix();

        // Inverse: VRC^-1 * Scale^-1
        core::mat4 inv_scale = core::getScalingMatrix(camera.view_width / 2.0f, camera.view_height / 2.0f, 1.0f);
        this->InverseNCSTransformMatrix = camera.GetInverseVRCMatrix() * inv_scale;
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
    } else {
        w.center = center;
        w.width  = width;
        w.height = height;
        w.angle  = angle;
    }
    return w;
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
        camera.zoom(zoom_factor);
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
        // Convert pixel delta to world-space delta via the inverse NCS matrix.
        core::Point p1 = ViewportToWindow(ImVec2(0.0f, 0.0f));
        core::Point p2 = ViewportToWindow(ImVec2(dx, dy));
        camera.vrp.x -= (p2.x - p1.x);
        camera.vrp.y -= (p2.y - p1.y);
        // Z (world up) is intentionally locked — panning only moves in the XY floor plane.
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

void Window::OnModeChanged() {
    if (AppConfig::is3d) {
        // Z-up convention: vpn = normalize(1,-1,1) puts camera at 315° azimuth, 35° elevation.
        // Result: X goes right-down, Y goes right-up, Z goes straight up on screen.
        camera.vrp        = {0.0f, 0.0f, 0.0f};
        camera.vpn        = {0.5774f, -0.5774f, 0.5774f};
        camera.vup        = {0.0f, 0.0f, 1.0f};
        camera.view_width  = 20.0f;
        camera.view_height = 20.0f;
    }
    UpdateNCSMatrix();
}