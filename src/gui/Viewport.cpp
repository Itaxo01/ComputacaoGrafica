#include "Viewport.hpp"

void Viewport::DrawWindow() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 monitor_pos = viewport->Pos;
    ImVec2 monitor_size = viewport->Size;
    
    // Proportional window configurations based on the app window/monitor size
    ImGui::SetNextWindowPos(ImVec2(monitor_pos.x + monitor_size.x * (29.0f / 1700.0f), monitor_pos.y + monitor_size.y * (18.0f / 940.0f)), ImGuiCond_FirstUseEver); 
    ImGui::SetNextWindowSize(ImVec2(monitor_size.x * (893.0f / 1700.0f), monitor_size.y * (857.0f / 940.0f)), ImGuiCond_FirstUseEver); 
    ImGui::Begin("Viewport");
        canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
        canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
        if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
        if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
        canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);


        canvas_sz.x -= 2*offset, canvas_sz.y -= 2*offset; // offset requerido na entrega 1.4
        canvas_p0.x += offset, canvas_p0.y += offset;
        canvas_p1.x -= offset, canvas_p1.y -= offset;
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
            if(ImGui::RadioButton("Liang Barsky Clipping", &clipping_mode, 0)){
                log.AddLog("Clipping mode changed to Liang Barsky Clipping\n");
            }
            if(ImGui::RadioButton("Cohen Sutherland Clipping", &clipping_mode, 1)){
                log.AddLog("Clipping mode changed to Cohen Sutherland Clipping\n");
            }
            // ImGui::Checkbox("Enable 3D visualization", &is3d);
        ImGui::EndChild();
 
    ImGui::End();

}