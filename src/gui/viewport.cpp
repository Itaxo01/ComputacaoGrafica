#include "viewport.h"
#include "EntityManager.hpp"

std::vector<std::pair<float, float>> ImVecToVec(ImVector<ImVec2> &p){
    std::vector<std::pair<float, float>> result;
    result.reserve(p.Size);
    for(int i = 0; i<p.Size; i++){
        result.emplace_back(p[i].x, p[i].y);
    }
    return result;
}

void Viewport::HandleLeftClick() {
    const float magic_constant = 5;
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 window_pos = ImGui::GetCursorScreenPos();
    float x = mouse_pos.x - window_pos.x;
    float y = window_pos.y - mouse_pos.y - magic_constant;
    log.AddLog("Canvas was clicked. Position = (%.1f, %.1f)\n", x, y);

    points.push_back(ImVec2(x, y));

    if (mode == Mode::POINT || (mode == Mode::LINE && points.size() == 2)) {
        AddGraphicObject();
    }
}

void Viewport::HandleRightDragging() {
    ImGuiIO& io = ImGui::GetIO();
    log.AddLog("Canvas is being dragged. dx = {%.1f}, dy = {%.1f}\n",
    io.MouseDelta.x, io.MouseDelta.y);
    // TO DO
}

void Viewport::HandlePointButtonClick() {
    log.AddLog("Point button was clicked.\n");
    points.clear();
    mode = Mode::POINT;
}

void Viewport::HandleLineButtonClick() {
    log.AddLog("Line button was clicked.\n");
    points.clear();
    mode = Mode::LINE;
}

void Viewport::HandleWireframeButtonClick() {
    log.AddLog("Wireframe button was clicked.\n");
    points.clear();
    mode = Mode::WIREFRAME;
}

void Viewport::HandleEnterButtonClick() {
    log.AddLog("Wireframe button was clicked.\n");
    if (points.size() > 2) {
        AddGraphicObject();
    } else {
        log.AddLog("[error] Cannot create Wireframe object with less than 3 points.\n");
        points.clear();
    }
}

ImVec2 Viewport::GetViewportSize() {
    return ImGui::GetContentRegionAvail();
}

ImDrawList* Viewport::GetDrawList() {
    return ImGui::GetWindowDrawList();
}

void Viewport::AddGraphicObject() {
    std::vector<std::pair<float, float>> points_vec = ImVecToVec(points);
    entityManager.add(points_vec); // precisa de um nome também
    points.clear();
}

void Viewport::run() {
    ImGui::Begin("Viewport");

    if (ImGui::Button("Point"))
        HandlePointButtonClick();
    if (ImGui::Button("Line"))
        HandleLineButtonClick();
    if (ImGui::Button("Wireframe"))
        HandleWireframeButtonClick();
    if (ImGui::Button("Enter"))
        HandleEnterButtonClick();

    ImGui::Text("Mouse Left: drag to add lines,\nMouse Right: drag to scroll0");

    // Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
    // Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
    // To use a child window instead we could use, e.g:
    //      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));      // Disable padding
    //      ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(50, 50, 50, 255));  // Set a background color
    //      ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
    //      ImGui::PopStyleColor();
    //      ImGui::PopStyleVar();
    //      [...]
    //      ImGui::EndChild();

    // Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

    // Draw border and background color
    // DEVE SER PASSADO PARA O RENDERER
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

    // This will catch our interactions
    ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool is_hovered = ImGui::IsItemHovered(); // Hovered
    const bool is_active = ImGui::IsItemActive();   // Held

    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        HandleLeftClick();

    const float mouse_threshold_for_pan = 0.0f;
    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan))
        HandleRightDragging();

    // Draw all lines in the canvas
    // PASSAR PARA O RENDERER
    /*draw_list->PushClipRect(canvas_p0, canvas_p1, true);
    for (int n = 0; n < points.Size; n += 2)
        draw_list->AddLine(ImVec2(origin.x + points[n].x, origin.y + points[n].y), ImVec2(origin.x + points[n + 1].x, origin.y + points[n + 1].y), IM_COL32(255, 255, 0, 255), 2.0f);
    draw_list->PopClipRect();*/
    ImGui::End();

    log.Draw("Log");
}