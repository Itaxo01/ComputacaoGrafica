#include "Viewport.hpp"

void Viewport::DrawWindow() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    
    // Hardcoded window configurations
    ImGui::SetNextWindowPos(ImVec2(work_pos.x + work_size.x * (38.0f / 1920.0f), work_pos.y + work_size.y * (25.0f / 1080.0f)), ImGuiCond_FirstUseEver); // Viewport window position
    ImGui::SetNextWindowSize(ImVec2(work_size.x * (832.0f / 1920.0f), work_size.y * (847.0f / 1080.0f)), ImGuiCond_FirstUseEver); // Viewport window size
    ImGui::Begin("Viewport");
        canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
        canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
        if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
        if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
        canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

        draw_list = ImGui::GetWindowDrawList();

        // This will catch our interactions
        ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        this->is_hovered = ImGui::IsItemHovered(); // Hovered
        this->is_active = ImGui::IsItemActive();   // Held
        
        ImGui::SetCursorScreenPos(ImVec2(canvas_p1.x - 210, canvas_p0.y + 5));
        ImGui::SetNextWindowBgAlpha(0.8f); // Slightly transparent background
       
        ImGui::BeginChild("Viewport Options", ImVec2(200, 115), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Checkbox("Show Axes", &show_axes);
            ImGui::Checkbox("Show Grid", &show_grid);
            ImGui::Checkbox("Show Axis Coordinates", &show_axis_coordinates);
            // ImGui::Checkbox("Enable 3D visualization", &is3d);
        ImGui::EndChild();
 
    ImGui::End();

}