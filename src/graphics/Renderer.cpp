#include "Renderer.hpp"
#include "imgui.h"

inline ImVec2 ToImVec2(const core::Point &p) {
    return ImVec2(p.x, p.y);
}

void Renderer::renderName(const core::Shape &shape){
    
    #ifdef DRAW_SHAPE_NAME
        core::Point anchor = core::Point(shape.anchorPoint());
        DrawObject(anchor);
        core::Point p = window.WorldToViewport(anchor);
        const int magic_number = 15;
        ImVec2 pos(p.x, p.y - magic_number);

        draw_list->AddText(pos, IM_COL32_WHITE, shape.name.c_str());

    #endif
}

// Draw border and background color
void Renderer::RenderBackground() {
    std::pair<ImVec2, ImVec2> canvas_p = viewport.GetCanvasP();
    ImVec2 canvas_p0 = canvas_p.first; ImVec2 canvas_p1 = canvas_p.second;
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));
}

void Renderer::DrawObject(const core::Point &world_p) {
    core::Point screen_p = window.WorldToViewport(world_p);

    const float rad = 2.5;
    draw_list->AddCircle(ToImVec2(screen_p), rad, IM_COL32_WHITE, 0, 2.0f);
}

void Renderer::DrawObject(const core::Line &line) {
    core::Point p0 = window.WorldToViewport(line.a); 
    core::Point p1 = window.WorldToViewport(line.b);

    const float width = 2.0f;
    draw_list->AddLine(ToImVec2(p0), ToImVec2(p1), IM_COL32_WHITE, width);
    //draw_list->PopClipRect(); // ver oq faz
}
void Renderer::DrawObject(const core::Wireframe &wireframe) {
    const float width = 2.0f;
    int size = wireframe.data.size();
    for (int i = 0; i < size-1; i++) {
        core::Point p0 = window.WorldToViewport(wireframe.data[i]); 
        core::Point p1 = window.WorldToViewport(wireframe.data[i+1]);

        draw_list->AddLine(ToImVec2(p0), ToImVec2(p1), IM_COL32_WHITE, width);
    }
}

void Renderer::render() {
    this->draw_list = viewport.GetDrawList();

    RenderBackground();
    for (const core::Point &point: displayFile.getPointList()) {
        DrawObject(point);
        renderName(point);
    }
    for (const core::Line &line: displayFile.getLineList()) {
        DrawObject(line);
        renderName(line);
    }
    for (const core::Wireframe &wireframe: displayFile.getWireframeList()) {
        DrawObject(wireframe);
        renderName(wireframe);
    }
    log.Draw("Log");
}