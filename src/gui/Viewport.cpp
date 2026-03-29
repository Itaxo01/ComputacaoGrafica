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
    ImGui::End();
}