#include "Viewport.hpp"

void Viewport::DrawWindow() {
    // Hardcoded window configurations
    ImGui::SetNextWindowPos(ImVec2(38, 25), ImGuiCond_FirstUseEver); // Viewport window position
    ImGui::SetNextWindowSize(ImVec2(832, 847), ImGuiCond_FirstUseEver); // Viewport window size
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
        
        ImGui::SetCursorScreenPos(ImVec2(canvas_p1.x - 190, canvas_p0.y + 5));
        ImGui::SetNextWindowBgAlpha(0.8f); // Slightly transparent background
       
        ImGui::BeginChild("Viewport Options", ImVec2(190, 85), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Checkbox("Show Axes", &show_axes);
            ImGui::Checkbox("Show Grid", &show_grid);
            ImGui::Checkbox("Show Axis Coordinates", &show_axis_coordinates);
            ImGui::Checkbox("Enable 3D visualization", &is3d);
        ImGui::EndChild();
 
    ImGui::End();

}