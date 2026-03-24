#include "Renderer.hpp"
inline ImVec2 ToImVec2(const core::Point &p) {
    return ImVec2(p.x, p.y);
}

// Draw border and background color
void Renderer::RenderBackground() {
    ImDrawList* draw_list = viewport.GetDrawList();

    std::pair<ImVec2, ImVec2> canvas_p = viewport.GetCanvasP();
    ImVec2 canvas_p0 = canvas_p.first; ImVec2 canvas_p1 = canvas_p.second;
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));
}

void Renderer::DrawObject(const core::Point &world_p) {
    ImDrawList* draw_list = viewport.GetDrawList();
    core::Point screen_p = window.WorldToViewport(world_p);

    const float rad = 2.5;
    draw_list->AddCircle(ToImVec2(screen_p), rad, IM_COL32_WHITE, 0, 2.0f);
}

void Renderer::DrawObject(const core::Line &line) {
    ImDrawList* draw_list = viewport.GetDrawList();
    
    core::Point p0 = window.WorldToViewport(line.a); 
    core::Point p1 = window.WorldToViewport(line.b);

    const float width = 2.0f;
    draw_list->AddLine(ToImVec2(p0), ToImVec2(p1), IM_COL32_WHITE, width);
    //draw_list->PopClipRect(); // ver oq faz
}
void Renderer::DrawObject(const core::Wireframe &wireframe) {
    ImDrawList* draw_list = viewport.GetDrawList();
    
    const float width = 2.0f;
    int size = wireframe.data.size();
    for (int i = 0; i < size-1; i++) {
        core::Point p0 = window.WorldToViewport(wireframe.data[i]); 
        core::Point p1 = window.WorldToViewport(wireframe.data[i+1]);

        draw_list->AddLine(ToImVec2(p0), ToImVec2(p1), IM_COL32_WHITE, width);
    }
}

void Renderer::render() {
    RenderBackground();

    for (const core::Point &point :displayFile.getPointList()) {
        DrawObject(point);
    }
    for (const core::Line &line :displayFile.getLineList()) {
        DrawObject(line);
    }
    for (const core::Wireframe &wireframe :displayFile.getWireframeList()) {
        DrawObject(wireframe);
    }
    log.Draw("Log");
}