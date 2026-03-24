#include "Viewport.hpp"
#include "Window.hpp"



void Viewport::HandleLeftClick() {
    ImVec2 mouse_pos = ImGui::GetMousePos();

    core::Point world_p = window->ViewportToWorld(mouse_pos);
    float x = world_p.x, y = world_p.y;

    
    if (enable_object_creation) {
        if (points.size() > 2 && mode == core::ShapeType::WIREFRAME &&
            x == points.back().x && y == points.back().y) {
            AddGraphicObject();
            return;
        }

        points.push_back(ImVec2(x, y));
        if (mode == core::ShapeType::POINT || (mode == core::ShapeType::LINE && points.size() == 2)) {
            AddGraphicObject();
        }
    }
}

void Viewport::HandleRightDragging() {
    ImGuiIO& io = ImGui::GetIO();
    log.AddLog("Canvas is being dragged. dx = {%.1f}, dy = {%.1f}\n",
    io.MouseDelta.x, io.MouseDelta.y);
    // move the canvas on the opposite direction
    window->moveWindow(-io.MouseDelta.x, io.MouseDelta.y, canvas_sz);
}

void Viewport::HandleScroll(const float delta){
    ImVec2 mouse_pos = ImGui::GetMousePos();
    // A Anchor vai dar um fator de % para cada dimensão do zoom
    std::string mode = delta > 0.0f ? "in" : delta < 0.0f ? "out" : "";
    log.AddLog("Canvas zoomed {%s}. delta = {%.1f} at position ({%.1f}, {%.1f})\n", mode.c_str(), delta, mouse_pos.x, mouse_pos.y);

    // move em 10%
    float zoom_factor = delta > 0.0f ? 0.9f : delta < 0.0f ? 1.1f : 1.0f;
    window->zoom(zoom_factor, mouse_pos);
}

void Viewport::DrawWindow() {
    // Hardcoded window configurations
    ImGui::SetNextWindowPos(ImVec2(38, 25), ImGuiCond_FirstUseEver); // Viewport window position
    ImGui::SetNextWindowSize(ImVec2(805, 700), ImGuiCond_FirstUseEver); // Viewport window size
    ImGui::Begin("Viewport");
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
        
        if (is_hovered) {
            float scroll = ImGui::GetIO().MouseWheel;
            if(scroll != 0.0f){
                HandleScroll(scroll);
            }
        }
    ImGui::End();
}
