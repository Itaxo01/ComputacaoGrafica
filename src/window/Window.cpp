#include "Window.hpp"
#include "Viewport.hpp"

void Window::setWorldBounds(core::Point &bottomLeft, core::Point &topRight){
    p0 = bottomLeft, p1 = topRight;
}

core::Point Window::WorldToViewport(core::Point wp) {
    std::pair<ImVec2, ImVec2> cp = viewport.GetCanvasP();
    float xvp = (wp.x - p0.x) / (p1.x - p0.x) * (cp.second.x - cp.first.x);
    float yvp = (1 - (wp.y - p0.y) / (p1.y - p0.y)) * (cp.second.y - cp.first.y);
    return core::Point(xvp, yvp);
}

core::Point Window::ViewportToWorld(ImVec2 vp) {
    std::pair<ImVec2, ImVec2> cp = viewport.GetCanvasP(); // subtrai o offset do canvas aqui.
    float xw = p0.x + (vp.x - cp.first.x) / (cp.second.x - cp.first.x) * (p1.x - p0.x);
    float yw = p0.y + (1 - (vp.y - cp.first.y) / (cp.second.y - cp.first.y)) * (p1.y - p0.y);
    return core::Point(xw, yw);
}
