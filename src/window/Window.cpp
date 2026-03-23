#include "Window.hpp"
#include "Viewport.hpp"

void Window::setWorldBounds(const core::Point &bottomLeft, const core::Point &topRight){
    p0 = bottomLeft, p1 = topRight;
}

core::Point Window::WorldToViewport(const core::Point &wp) {
    std::pair<ImVec2, ImVec2> cp = viewport.GetCanvasP();
    ImVec2 canvas_sz = viewport.GetCanvasSize();
    // First, we normalize the coordinates like normal_x = (wp.x - p0.x) / (p1.x - p0.x) (between [-1, 1]), and same but inverted for y.
    // second, we scale this by the canvas size
    // and lastly, we add the screen offset
    return core::Point(
        ((wp.x - p0.x) / (p1.x - p0.x))*canvas_sz.x + cp.first.x,
        (1.0f - ((wp.y - p0.y) / (p1.y - p0.y)))*canvas_sz.y + cp.first.y
    );
}

core::Point Window::ViewportToWorld(const ImVec2 &vp) {
    std::pair<ImVec2, ImVec2> cp = viewport.GetCanvasP(); // subtrai o offset do canvas aqui.
    ImVec2 canvas_sz = viewport.GetCanvasSize();
    // Do the same steps as WorldToViewport, but inverted.
    return core::Point(
        ((vp.x - cp.first.x)/canvas_sz.x) * (p1.x - p0.x) + p0.x, 
        (1.0f - ((vp.y - cp.first.y)/canvas_sz.y)) * (p1.y - p0.y) + p0.y
    );
}


void Window::zoom(const float zoom_factor, const ImVec2 &mouse_pos){
    // 1. Get the current World Coordinate the mouse is hovering over
    core::Point world_anchor = ViewportToWorld(mouse_pos);

    // 2. Calculate the exact normalized position of the mouse inside the Window (0.0 to 1.0)
    // This tells us if it's 20% from the left, 90% from the top, etc.
    float nx = (world_anchor.x - p0.x) / (p1.x - p0.x);
    float ny = (world_anchor.y - p0.y) / (p1.y - p0.y);

    // 3. Calculate the new width and height based on the zoom factor
    // E.g., if zoom_factor is 0.9 (zoom in), new width is 90% of old width
    float new_width = (p1.x - p0.x) * zoom_factor;
    float new_height = (p1.y - p0.y) * zoom_factor;

    // 4. Reposition p0 and p1 so the anchor point remains at the exact same 'nx' and 'ny' percentage
    p0.x = world_anchor.x - (new_width * nx);
    p1.x = p0.x + new_width;

    p0.y = world_anchor.y - (new_height * ny);
    p1.y = p0.y + new_height;
}

void Window::moveWindow(const float dx, const float dy, const ImVec2 &canvas_sz){
    float x = dx*((p1.x - p0.x)/canvas_sz.x);
    float y = dy*((p1.y - p0.y)/canvas_sz.y);
    p0.x += x, p1.x += x, p0.y += y, p1.y += y;
}

