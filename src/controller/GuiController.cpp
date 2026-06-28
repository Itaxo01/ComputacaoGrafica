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
    ImGuiIO& io = ImGui::GetIO();
    float sv = io.MouseWheel;   // touchpad vertical pan (up/down)
    float sh = io.MouseWheelH;  // touchpad horizontal pan (left/right)
    if (sv == 0.0f && sh == 0.0f) return;

    ImVec2 mouse_pos = ImGui::GetMousePos();
    const bool ctrl = io.KeyCtrl, shift = io.KeyShift;

    if (ctrl && shift) {
        // Rotate, like Ctrl+Shift+arrows. 3D has two DOF (horizontal = yaw,
        // vertical = pitch); 2D has a single rotation, driven by horizontal.
        if (sh != 0.0f) {
            float deg = sh * 5.0f;
            window.rotate(deg);
            log.AddLog("Rotated %s by %.1f deg\n",
                       AppConfig::is3d ? "camera yaw" : "window", deg);
        }
        if (AppConfig::is3d && sv != 0.0f) {
            float deg = -sv * 5.0f;
            window.orbitPitch(deg);
            log.AddLog("Camera pitch %.1f deg\n", deg);
        }
    } else if (shift) {
        // Translate in both axes — same directions as Shift+arrows, bigger delta.
        float dx = -sh * 30.0f, dy = sv * 30.0f;
        window.moveWindow(dx, dy, viewport.GetCanvasSize());
        log.AddLog("Translated window ({%.0f}, {%.0f})\n", dx, dy);
    } else if (ctrl && AppConfig::is3d && AppConfig::perspective && sv != 0.0f) {
        // Perspective COP (wide-angle / telephoto) — relocated here from Shift.
        float factor = sv > 0.0f ? 0.8f : 1.25f;
        window.adjustPerspective(factor);
        log.AddLog("Perspective COP %s\n", sv > 0.0f ? "telephoto" : "wide angle");
    } else if (sv != 0.0f) {
        // Plain vertical pan zooms (anchored at the cursor). Horizontal-only is ignored.
        float factor = sv > 0.0f ? 0.9f : 1.1f;
        window.zoom(factor, mouse_pos);
        log.AddLog("Canvas zoomed {%s} at ({%.1f}, {%.1f})\n",
                   sv > 0.0f ? "in" : "out", mouse_pos.x, mouse_pos.y);
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

        ImGuiIO& io = ImGui::GetIO();
        if (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f) {
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
