#include "viewport.h"

std::vector<std::pair<float, float>> Viewport::ImVecToVec(ImVector<ImVec2> &p){
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

    if (enable_object_creation) {
        if (points.size() > 2 && mode == ShapeType::WIREFRAME &&
            x == points[points.size()-1].x && y == points[points.size()-1].y) {
            AddGraphicObject();
            return;
        }

        points.push_back(ImVec2(x, y));
        if (mode == ShapeType::POINT || (mode == ShapeType::LINE && points.size() == 2)) {
            AddGraphicObject();
        }
    }
}

void Viewport::HandleRightDragging() {
    ImGuiIO& io = ImGui::GetIO();
    log.AddLog("Canvas is being dragged. dx = {%.1f}, dy = {%.1f}\n",
    io.MouseDelta.x, io.MouseDelta.y);
    // TO DO
}

void Viewport::AddGraphicObject() {
    std::string name(obj_name);
    if (name == "" || name == "\1") { // Substituir por um regex depois...
        log.AddLog("[error] Cannot create object (Invalid name): {%s}\n", obj_name);
        return;
    }
    log.AddLog("Creating new object... name: {%s}\n", obj_name);
    std::vector<std::pair<float, float>> points_vec = ImVecToVec(points);
    entityManager.add(name, points_vec);
    points.clear();
}

void Viewport::run() {
    ImGui::Begin("Viewport");

    // OBJECT CREATION
    ImGui::Begin("Create New Object");
        if (ImGui::Checkbox("Enable Object Creation:", &enable_object_creation)) {
            points.clear();
        }
        if (ImGui::RadioButton("Point", &e, 0)) {
            log.AddLog("Mode changed to POINT\n");
            mode = ShapeType::POINT;
            points.clear();
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Line", &e, 1)) {
            log.AddLog("Mode changed to LINE\n");
            mode = ShapeType::LINE;
            points.clear();
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Wireframe", &e, 2)) {
            log.AddLog("Mode changed to WIREFRAME\n");
            mode = ShapeType::WIREFRAME;
            points.clear();
        }
        // BEGIN INPUT TEXT
        ImGui::Text("Object name:"); ImGui::SameLine();
        ImGui::InputText("##", obj_name, IM_COUNTOF(obj_name));
        // END INPUT TEXT
        ImGui::Text("To create a point: \nclick on canvas 1 time\n");
        ImGui::Text("To create a line: \nClick on canvas 2 times\n");
        ImGui::Text("To create a wireframe: \nClick on canvas at least 3 times and\nclick on the same location again.\n");
    ImGui::End();

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
    canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
    canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
    canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

    // DEVE SER PASSADO PARA O RENDERER
    draw_list = ImGui::GetWindowDrawList();

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