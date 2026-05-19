#pragma once
#include "Shape.hpp"
#include "imgui.h"
#include "log_app.h"
#include <vector>
#include <tuple>
#include <string>

// Modal window for text-based object creation.
// Usage:
//   if (ImGui::Button("Create by text...")) text_creator.Open();
//   if (text_creator.DrawModal(mode, method, filled, out)) { points = out; AddGraphicObject(); }
class ObjectCreatorText {
    static constexpr int BUF_SIZE = 4096;

    char        buffer[BUF_SIZE] = "";
    bool        open_requested   = false;
    const char* popup_title;

    // Internal mode state — seeded by Open(), editable inside the modal
    core::ShapeType mode   = core::ShapeType::POINT;
    int             method = 0;     // 0=Bezier, 1=B-Spline (Curve2D only)
    bool            filled = false; // Polygon only
    int             e      = 0;     // radio index mirroring mode

    std::vector<std::tuple<float, float, float>> parsed;
    std::string parse_error;
    bool        parse_ok = false;

    ExampleAppLog &log;

    void        reparse();
    bool        validate() const;
    const char* format_hint() const;
    std::string validation_msg() const;
    void        draw_mode_selector();

public:
    explicit ObjectCreatorText(ExampleAppLog &log, const char* title = "Create by Text")
        : popup_title(title), log(log) {}

    // Seed internal mode from the parent before opening.
    void Open(core::ShapeType initial_mode, int initial_method = 0, bool initial_filled = false);

    // Pre-fill the buffer from existing points, then open.
    void OpenForEdit(core::ShapeType mode, int method, bool filled,
                     const std::vector<std::tuple<float, float, float>> &pts);

    // Call every frame inside the parent window. Returns true once on Confirm.
    // On confirm, out_* carry the (possibly changed) mode back to the caller.
    bool DrawModal(std::vector<std::tuple<float, float, float>> &out_points,
                   core::ShapeType &out_mode, int &out_method, bool &out_filled);
};
