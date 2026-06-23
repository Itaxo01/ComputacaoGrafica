#ifndef GUI_LAYOUT_HPP
#define GUI_LAYOUT_HPP

#include "imgui.h"

// Single source of truth for the default placement of the app's main windows.
//
// Instead of every window hardcoding its geometry as fractions of a 1700x940
// reference (which distorts across aspect ratios and can spill off-screen), the
// layout is modelled here as a fractional grid over the live main-viewport work
// area. Each region is clamped to a per-region minimum size and to the work-area
// bounds, so every window is guaranteed to start fully visible at any resolution.
//
// Geometry is still applied with ImGuiCond_FirstUseEver (via Cond()), so the user
// keeps full freedom to move/resize; RequestReset() snaps everything back to the
// computed defaults without having to delete imgui.ini by hand.
namespace gui::layout {

enum class Region {
    Viewport,       // big left column, full height
    CreateObject,   // top-right
    ObjectManager,  // mid-right
    Lighting,       // bottom-right, left half
    Log,            // bottom-right, right half
};

struct Rect { ImVec2 pos, size; };

// DPI / font scale, so minimum sizes and margins grow on hi-DPI displays.
// Call once from InitializeImGui after the scale is known. Defaults to 1.0.
void SetScale(float scale);

// The DPI scale set above. Multiply fixed pixel sizes of inner child panes by
// this so they track the font/DPI scale and don't clip on hi-DPI displays.
float Scale();

// Default rect for a region in absolute screen coords, clamped to the main
// viewport work area and to the region's minimum size. Recomputed each call.
Rect Get(Region r);

// Re-apply the default layout: makes Cond() return ImGuiCond_Always for this
// frame and the next, overriding any saved imgui.ini positions for one snap.
void RequestReset();

// Condition to feed SetNextWindowPos/Size: Always while a reset is pending,
// FirstUseEver otherwise.
ImGuiCond Cond();

} // namespace gui::layout

#endif // GUI_LAYOUT_HPP
