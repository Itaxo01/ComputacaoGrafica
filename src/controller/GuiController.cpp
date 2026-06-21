#include "GuiController.hpp"
#include "AppConfig.hpp"
#include "imgui.h"

void GuiController::run(){
    viewport.DrawWindow();
    creator.DrawWindow();
    objGUI.DrawWindow();
    log.Draw("Log");

    if (AppConfig::is3d != prev_is3d) {
        prev_is3d = AppConfig::is3d;
        window.OnModeChanged();
        log.AddLog("Switched to %s mode\n", AppConfig::is3d ? "3D" : "2D");
    }

    if (AppConfig::perspective != prev_perspective) {
        prev_perspective = AppConfig::perspective;
        // Rebuild the NCS matrix now; the render cache invalidates itself via
        // WindowAttributes (which tracks AppConfig::perspective).
        window.OnPerspectiveChanged();
    }

    HandleCanvasInteractions();
}

// ─── Canvas input ─────────────────────────────────────────────────────────────

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
    window.moveWindow(io.MouseDelta.x, io.MouseDelta.y, viewport.GetCanvasSize());
}

void GuiController::HandleScroll(){
    float scroll = ImGui::GetIO().MouseWheel;
    if (scroll == 0.0f) return;

    ImVec2 mouse_pos = ImGui::GetMousePos();

    // Shift + scroll in 3D perspective moves the Centre of Projection
    // (focal distance) → wide-angle / telephoto distortion.
    // Plain scroll always zooms the view (works in perspective too).
    if (AppConfig::is3d && AppConfig::perspective && ImGui::GetIO().KeyShift) {
        // Larger step than zoom: the default focal distance is large, so a
        // gentle factor would need many scrolls to reach a visible distortion.
        float factor = scroll > 0.0f ? 0.8f : 1.25f;
        window.adjustPerspective(factor);
        log.AddLog("Perspective COP %s (%s)\n",
                   scroll > 0.0f ? "moved back" : "moved closer",
                   scroll > 0.0f ? "telephoto" : "wide angle");
    } else {
        float factor = scroll > 0.0f ? 0.9f : 1.1f;
        window.zoom(factor, mouse_pos);
        log.AddLog("Canvas zoomed {%s}. scroll = {%.1f} at position ({%.1f}, {%.1f})\n",
                   scroll > 0.0f ? "in" : "out", scroll, mouse_pos.x, mouse_pos.y);
    }
}

void GuiController::HandleKeyboard(){
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && io.KeyShift) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
            if (AppConfig::is3d) { log.AddLog("Camera orbit yaw -1 degree\n"); }
            else                 { log.AddLog("Rotated window 1 degree counter-clockwise\n"); }
            window.rotate(-1.0f);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
            if (AppConfig::is3d) { log.AddLog("Camera orbit yaw +1 degree\n"); }
            else                 { log.AddLog("Rotated window 1 degree clockwise\n"); }
            window.rotate(1.0f);
        }
        if (AppConfig::is3d) {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
                log.AddLog("Camera orbit pitch -1 degree\n");
                window.orbitPitch(-1.0f);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
                log.AddLog("Camera orbit pitch +1 degree\n");
                window.orbitPitch(1.0f);
            }
        }
    }
    else if(io.KeyShift){
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
    else if(io.KeyCtrl) {
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

void GuiController::HandleCanvasInteractions(){
    bool is_active  = viewport.IsActive();
    bool is_hovered = viewport.IsHovered();

    // Shape-finishing keys fire even if the cursor drifted slightly off the canvas.
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
        creator.CloseShape();
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        creator.CancelCreation();

    if(is_hovered){
        // Click-to-add is 2D only; in 3D objects are created via text creation.
        if (!AppConfig::is3d) {
            // Double-click closes wireframe/polygon without adding an extra vertex.
            // Check before single-click so the else-if suppresses the regular click.
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                creator.CloseShape();
            } else if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
                HandleLeftClick();
            }
        }

        float scroll = ImGui::GetIO().MouseWheel;
        if(scroll != 0.0f){
            HandleScroll();
        }
        HandleKeyboard();
    }
    if(is_active){
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)){
            HandleRightDragging();
        }
    }
}
