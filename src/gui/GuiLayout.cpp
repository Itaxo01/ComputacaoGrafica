#include "GuiLayout.hpp"

#include <algorithm>

namespace gui::layout {

namespace {
    float g_scale = 1.0f;
    // Frame index up to which Cond() forces ImGuiCond_Always. -1 = no reset pending.
    int   g_reset_until_frame = -1;

    // Grid tuning (fractions of the inner work area). Mirrors the original
    // two-column layout: a wide Viewport on the left, three stacked panels on
    // the right, the bottom one split between Lighting and Log.
    constexpr float kLeftColFrac = 0.55f;   // Viewport share of the inner width
    constexpr float kRowCreate   = 0.22f;   // right column: top    (Create New Object)
    constexpr float kRowManager  = 0.54f;   // right column: middle (Object Manager)
    constexpr float kRowBottom   = 0.24f;   // right column: bottom (Lighting | Log)

    float clampf(float v, float lo, float hi) {
        return hi < lo ? lo : std::min(std::max(v, lo), hi);
    }

    // Clamp a rect to its minimum size and keep it fully inside [origin, origin+avail].
    Rect finalize(ImVec2 origin, ImVec2 avail, ImVec2 pos, ImVec2 size, ImVec2 min) {
        size.x = clampf(size.x, min.x * g_scale, avail.x);
        size.y = clampf(size.y, min.y * g_scale, avail.y);
        pos.x  = clampf(pos.x, origin.x, origin.x + avail.x - size.x);
        pos.y  = clampf(pos.y, origin.y, origin.y + avail.y - size.y);
        return { pos, size };
    }
}

void SetScale(float scale) { g_scale = scale > 0.0f ? scale : 1.0f; }

float Scale() { return g_scale; }

void RequestReset() { g_reset_until_frame = ImGui::GetFrameCount() + 1; }

ImGuiCond Cond() {
    return ImGui::GetFrameCount() <= g_reset_until_frame
               ? ImGuiCond_Always
               : ImGuiCond_FirstUseEver;
}

Rect Get(Region r) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 origin = vp->WorkPos;   // respects any main menu bar
    const ImVec2 work   = vp->WorkSize;

    const float m = 8.0f * g_scale;   // outer margin
    const float g = 6.0f * g_scale;   // gutter between cells

    const float innerW = std::max(work.x - 2.0f * m, 1.0f);
    const float innerH = std::max(work.y - 2.0f * m, 1.0f);

    // Column split.
    const float leftW  = (innerW - g) * kLeftColFrac;
    const float rightW = (innerW - g) - leftW;
    const float rightX = origin.x + m + leftW + g;

    // Right-column row heights.
    const float rowsH   = innerH - 2.0f * g;
    const float hCreate = rowsH * kRowCreate;
    const float hManage = rowsH * kRowManager;
    const float hBottom = rowsH * kRowBottom;

    const float topY    = origin.y + m;
    const float yManage = topY + hCreate + g;
    const float yBottom = yManage + hManage + g;
    const float halfW   = (rightW - g) * 0.5f;

    switch (r) {
    case Region::Viewport:
        return finalize(origin, work,
                        { origin.x + m, topY }, { leftW, innerH }, { 400, 300 });
    case Region::CreateObject:
        return finalize(origin, work,
                        { rightX, topY }, { rightW, hCreate }, { 320, 140 });
    case Region::ObjectManager:
        return finalize(origin, work,
                        { rightX, yManage }, { rightW, hManage }, { 360, 340 });
    case Region::Lighting:
        return finalize(origin, work,
                        { rightX, yBottom }, { halfW, hBottom }, { 240, 160 });
    case Region::Log:
        return finalize(origin, work,
                        { rightX + halfW + g, yBottom }, { halfW, hBottom }, { 240, 160 });
    }
    // Unreachable; keeps the compiler happy.
    return finalize(origin, work, origin, work, { 100, 100 });
}

} // namespace gui::layout
