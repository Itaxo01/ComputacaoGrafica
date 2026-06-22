#include "Viewport.hpp"
#include "AppConfig.hpp"
#include "Lighting.hpp"

// Lighting / shading controls. Standalone ImGui window so the multi-light editor
// has room; the globals live in Lighting:: (read live by the renderer each frame).
static void DrawLightingWindow() {
    // Left column of the bottom row, mirroring the Log window to its right.
    const ImGuiViewport* mv = ImGui::GetMainViewport();
    ImVec2 monitor_pos = mv->Pos, monitor_size = mv->Size;
    ImGui::SetNextWindowPos(ImVec2(monitor_pos.x + monitor_size.x * (900.0f / 1700.0f), monitor_pos.y + monitor_size.y * (611.0f / 940.0f)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(monitor_size.x * (360.0f / 1700.0f), monitor_size.y * (227.0f / 940.0f)), ImGuiCond_FirstUseEver);

    ImGui::Begin("Lighting");

    const char* modes[] = { "None", "Flat", "Gouraud", "Phong" };
    ImGui::Combo("Shading Model", &Lighting::mode, modes, IM_ARRAYSIZE(modes));
    if (!AppConfig::is3d)
        ImGui::TextDisabled("(shading is intended for 3D mode)");
    ImGui::ColorEdit3("Ambient", &Lighting::ambient.r);

    ImGui::Separator();
    ImGui::Checkbox("Headlight", &Lighting::headlight);
    if (Lighting::headlight) {
        ImGui::ColorEdit3("Headlight color", &Lighting::headlight_color.r, ImGuiColorEditFlags_NoInputs);
        ImGui::DragFloat("Headlight intensity", &Lighting::headlight_intensity, 0.01f, 0.0f, 10.0f);
    }

    ImGui::Separator();
    ImGui::Text("Point lights (%d)", (int)Lighting::lights.size());
    if (ImGui::Button("Add light")) Lighting::lights.push_back(core::Light{});

    int to_remove = -1;
    for (int i = 0; i < (int)Lighting::lights.size(); ++i) {
        ImGui::PushID(i);
        core::Light& L = Lighting::lights[i];
        ImGui::Checkbox("##en", &L.enabled); ImGui::SameLine();
        ImGui::SetNextItemWidth(180);
        ImGui::DragFloat3("pos", &L.position.x, 0.1f); ImGui::SameLine();
        ImGui::ColorEdit3("##col", &L.color.r, ImGuiColorEditFlags_NoInputs); ImGui::SameLine();
        ImGui::SetNextItemWidth(70);
        ImGui::DragFloat("##int", &L.intensity, 0.01f, 0.0f, 10.0f); ImGui::SameLine();
        if (ImGui::SmallButton("X")) to_remove = i;
        ImGui::PopID();
    }
    if (to_remove >= 0) Lighting::lights.erase(Lighting::lights.begin() + to_remove);

    ImGui::End();
}

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
        
        ImGui::SetCursorScreenPos(ImVec2(canvas_p1.x - 230, canvas_p0.y + 5));
        ImGui::SetNextWindowBgAlpha(0.8f); // Slightly transparent background
       
        ImGui::BeginChild("Viewport Options", ImVec2(215, 270), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
            ImGui::Checkbox("Show Axes", &show_axes);
            ImGui::Checkbox("Show Grid", &show_grid);
            ImGui::Checkbox("Show Axis Coordinates", &show_axis_coordinates);
            if (AppConfig::is3d)
                ImGui::Checkbox("Show Bounding Box", &AppConfig::show_bounding_box);
            if(ImGui::RadioButton("Liang Barsky Clipping", &clipping_mode, 0)){
                log.AddLog("Clipping mode changed to Liang Barsky Clipping\n");
            }
            if(ImGui::RadioButton("Cohen Sutherland Clipping", &clipping_mode, 1)){
                log.AddLog("Clipping mode changed to Cohen Sutherland Clipping\n");
            }
            if(ImGui::Checkbox("Enable 3D", &AppConfig::is3d)){
                log.AddLog("3D mode %s\n", AppConfig::is3d ? "enabled" : "disabled");
            }
            if(AppConfig::is3d && ImGui::Checkbox("Enable Perspective", &AppConfig::perspective)){
                log.AddLog("Perspective mode %s\n", AppConfig::perspective ? "enabled" : "disabled");
            }
            ImGui::Checkbox("Render Names", &AppConfig::render_names);
            if (AppConfig::is3d && ImGui::Checkbox("Z-Buffer (depth test)", &AppConfig::z_buffer)) {
                log.AddLog("Z-buffer %s\n", AppConfig::z_buffer ? "enabled" : "disabled (painter's)");
            }
            ImGui::SetNextItemWidth(110);
            if (ImGui::SliderInt("Anti-aliasing", &AppConfig::supersample, 1, 4, "SSAA %dx")) {
                if (AppConfig::supersample < 1) AppConfig::supersample = 1;
                log.AddLog("Supersampling set to %dx\n", AppConfig::supersample);
            }
        ImGui::EndChild();

    ImGui::End();

    DrawLightingWindow();
}