#include "Renderer.hpp"

// Draw border and background color
void Renderer::RenderBackground() {
    std::pair<ImVec2, ImVec2> canvas_p = viewport.GetCanvasP();
    ImVec2 canvas_p0 = canvas_p.first; ImVec2 canvas_p1 = canvas_p.second;
    ImDrawList* draw_list = viewport.GetDrawList();
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));
}

void Renderer::DrawObject(core::Point p) {
    ImDrawList* draw_list = viewport.GetDrawList();
    std::pair<ImVec2, ImVec2> cp = viewport.GetCanvasP();
    ImVec2 canvas_sz = viewport.GetCanvasSize();

    const int rad = 5;
    draw_list->AddCircle(ImVec2(p.x + cp.first.x, canvas_sz.y - p.y + cp.first.y), rad, IM_COL32_WHITE);
    return;
}

void Renderer::DrawObject(core::Line line) {
    ImDrawList* draw_list = viewport.GetDrawList();
    std::pair<ImVec2, ImVec2> cp = viewport.GetCanvasP();
    ImVec2 canvas_sz = viewport.GetCanvasSize();
    
    const float width = 2.0f;
    core::Point p0 = line.a; core::Point p1 = line.b;
    draw_list->AddLine(ImVec2(p0.x + cp.first.x, canvas_sz.y - p0.y + cp.first.y), ImVec2(cp.first.x + p1.x, cp.first.y + canvas_sz.y - p1.y), IM_COL32_WHITE, width);
    //draw_list->PopClipRect(); // ver oq faz
}
void Renderer::DrawObject(core::Wireframe wireframe) {
    ImDrawList* draw_list = viewport.GetDrawList();
    std::pair<ImVec2, ImVec2> cp = viewport.GetCanvasP();
    ImVec2 canvas_sz = viewport.GetCanvasSize();
    
    const float width = 2.0f;
    int size = wireframe.data.size();
    for (int i = 0; i < size; i++) {
        core::Point p0 = wireframe.data[i]; core::Point p1 = wireframe.data[i%size];
        draw_list->AddLine(ImVec2(p0.x + cp.first.x, canvas_sz.y - p0.y + cp.first.y), ImVec2(cp.first.x + p1.x, cp.first.y + canvas_sz.y - p1.y), IM_COL32_WHITE, width);
    }
}

void Renderer::render() {
    RenderBackground();
    // TO DO (fazer point, line e wireframe herdar da mesma classe)
    // Para usar o polimorfismo de DrawObject
    std::vector<core::Point> pointList = displayFile.getPointList();
    for (core::Point point : pointList) {
        DrawObject(point);
    }
    std::vector<core::Line> lineList = displayFile.getLineList();
    for (core::Line line : lineList) {
        DrawObject(line);
    }
    std::vector<core::Wireframe> wireframeList = displayFile.getWireframeList();
    for (core::Wireframe wireframe : wireframeList) {
        DrawObject(wireframe);
    }
    log.Draw("Log");
}