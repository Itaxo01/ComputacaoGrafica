#include "GuiController.hpp"
#include "imgui.h"

void GuiController::run(){
    viewport.DrawWindow();
    creator.DrawWindow();
    //listener.DrawWindow();
    objGUI.DrawWindow();
    log.Draw("Log");

    HandleCanvasInteractions();
}

void GuiController::HandleLeftClick(){
    ImVec2 mouse_pos = ImGui::GetMousePos();
    log.AddLog("Canvas clicked at. x = {%.1f}, y = {%.1f}\n", mouse_pos.x, mouse_pos.y);

    core::Point world_p = window.ViewportToWindow(mouse_pos);
    creator.RegisterLeftClick(world_p.x, world_p.y);
}

void GuiController::HandleRightDragging(){
    ImGuiIO& io = ImGui::GetIO();
    log.AddLog("Canvas is being dragged. dx = {%.1f}, dy = {%.1f}\n",
    io.MouseDelta.x, io.MouseDelta.y);
    // move the canvas on the opposite direction
    window.moveWindow(io.MouseDelta.x, io.MouseDelta.y, viewport.GetCanvasSize());
}

void GuiController::HandleScroll(){
    float scroll = ImGui::GetIO().MouseWheel;
    if (scroll != 0.0f) {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        // A Anchor vai dar um fator de % para cada dimensão do zoom
        std::string mode = scroll > 0.0f ? "in" : scroll < 0.0f ? "out" : "";
        log.AddLog("Canvas zoomed {%s}. scroll = {%.1f} at position ({%.1f}, {%.1f})\n", mode.c_str(), scroll, mouse_pos.x, mouse_pos.y);

        // move em 10%
        float zoom_factor = scroll > 0.0f ? 0.9f : scroll < 0.0f ? 1.1f : 1.0f;
        window.zoom(zoom_factor, mouse_pos);
    }
}

void GuiController::HandleKeyboard(){
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && io.KeyShift) {
        { // Girar window -> Ctrl + Shift + Left Arrow (Counter-clockwise)
            //               Ctrl + Shift + Right Arrow (Clockwise)
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
                log.AddLog("Rotated window 1 degree counter-clockwise\n");
                window.rotate(-1.0f); 
            }
            
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
                log.AddLog("Rotated window 1 degree clockwise\n");
                window.rotate(1.0f);
            }
        }
    }
    else if(io.KeyShift){
        { // Move window -> Shift + Arrow Key
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
                log.AddLog("Moving window up\n");
                window.moveWindow(0.0f, 5.0f, viewport.GetCanvasSize());
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
                log.AddLog("Moving window down\n");
                window.moveWindow(0.0f, -5.0f, viewport.GetCanvasSize());
            }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
                log.AddLog("Moving window left\n");
                window.moveWindow(5.0f, 0.0f, viewport.GetCanvasSize());
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
                log.AddLog("Moving window right\n");
                window.moveWindow(-5.0f, 0.0f, viewport.GetCanvasSize());
            }
        }
    }
    else if(io.KeyCtrl) {
        { // Zoom window -> Ctrl + UpArrow (zoom in)
            //           Ctrl + DownArrow (zoom out)
            auto cp = viewport.GetCanvasP();
            ImVec2 center_pos(
                cp.first.x + viewport.GetCanvasSize().x / 2.0f,
                cp.first.y + viewport.GetCanvasSize().y / 2.0f
            );
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
                log.AddLog("Zooming window in\n");
                window.zoom(1.1f, center_pos);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
                log.AddLog("Zooming window out\n");
                window.zoom(0.9f, center_pos);
            }
        }
    }

}


void GuiController::HandleCanvasInteractions(){
    bool is_active = viewport.IsActive();
    bool is_hovered = viewport.IsHovered();
    const float mouse_threshold_for_pan = 0.0f;

    if(is_hovered){
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
            HandleLeftClick();
        } 
        
        float scroll = ImGui::GetIO().MouseWheel;
        if(scroll != 0.0f){
            HandleScroll();
        }
        HandleKeyboard();
    }
    if(is_active){
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan)){
            HandleRightDragging();
        }
    }
}