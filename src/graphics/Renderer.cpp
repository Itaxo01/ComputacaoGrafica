#include "Renderer.hpp"

// Draw border and background color
void Renderer::RenderBackground() {
    std::pair<ImVec2, ImVec2> canvas_p = viewport.GetCanvasP();
    ImVec2 canvas_p0 = canvas_p.first; ImVec2 canvas_p1 = canvas_p.second;
    ImDrawList* draw_list = viewport.GetDrawList();
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));
}

void Renderer::DrawObject(core::Point point) {
    // TO DO
    return;
}

void Renderer::render() {
    RenderBackground();
    // TO DO
    std::vector<core::Point> pointList = displayFile.getPointList();
    for (core::Point point : pointList) {
        DrawObject(point);
    }
}