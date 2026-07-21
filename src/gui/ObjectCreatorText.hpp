#pragma once
#include "Object.hpp"
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
    core::ObjectType mode   = core::ObjectType::POINT;
    int             method = 0;     // 0=Bezier, 1=B-Spline (Curve2D only)
    bool            filled = false; // Polygon and surface
    bool            target_3d = false; // which object set the selector offers;
                                       // independent of AppConfig::is3d

    std::vector<std::tuple<float, float, float>> parsed;
    std::string parse_error;
    bool        parse_ok = false;

    // Surface grid shape, derived from ';'-separated rows during parse. cols is the
    // length of each row (all rows must match for a valid M×N control grid).
    int  grid_rows = 0;
    int  grid_cols = 0;
    bool grid_uniform = false;

    ExampleAppLog &log;

    void        reparse();
    bool        validate() const;
    const char* format_hint() const;
    std::string validation_msg() const;
    void        draw_mode_selector();

public:
    explicit ObjectCreatorText(ExampleAppLog &log, const char* title = "Create by Text")
        : popup_title(title), log(log) {}

    // Seed internal mode from the parent before opening. `initial_3d` seeds the
    // modal's own 2D/3D selector (which the user can then flip independently).
    void Open(core::ObjectType initial_mode, int initial_method = 0,
              bool initial_filled = false, bool initial_3d = false);

    // Pre-fill the buffer from existing points, then open. For surfaces, pass the
    // control-grid column count so rows are re-emitted with ';' separators (lets
    // the M×N grid round-trip through editing); 0 keeps one point per line.
    void OpenForEdit(core::ObjectType mode, int method, bool filled,
                     const std::vector<std::tuple<float, float, float>> &pts,
                     int grid_cols = 0);

    // Call every frame inside the parent window. Returns true once on Confirm.
    // On confirm, out_* carry the (possibly changed) mode back to the caller;
    // out_rows/out_cols carry the surface control-grid shape (0 for non-surfaces).
    bool DrawModal(std::vector<std::tuple<float, float, float>> &out_points,
                   core::ObjectType &out_mode, int &out_method, bool &out_filled,
                   int &out_rows, int &out_cols);
};
