#include "ObjectCreator.hpp"
#include "ObjectIO.hpp"
#include "AppConfig.hpp"
#include "GuiLayout.hpp"
#include "imgui.h"
#include "ObjectFactories/PointFactory.hpp"
#include "ObjectFactories/LineFactory.hpp"
#include "ObjectFactories/WireframeFactory.hpp"
#include "ObjectFactories/PolygonFactory.hpp"
#include "ObjectFactories/Curve2DFactory.hpp"
#include "ObjectFactories/SurfaceFactory.hpp"
#include <fstream>
#include <sstream>
#include <cstring>

// ─── Dynamic instruction text ────────────────────────────────────────────────

static const char* point_instruction() {
    return "Click on the canvas to place a point.\n"
           "Or open the Create by Text modal\n";
}

static const char* line_instruction(int n) {
    if (n == 0) return "Click to place the first endpoint.\n"
                       "Or open the Create by Text modal\n";
    return "Click to place the second endpoint.";
}

static const char* wireframe_instruction(int n) {
    if (n == 0) return "Click to place the first vertex.\n"
                       "Or open the Create by Text modal\n";
    if (n == 1) return "Click to place more vertices.\nPress Enter or double-click to finish.";
    return "Click to add vertices.\nPress Enter or double-click to finish.\nEsc to cancel.";
}

static const char* polygon_instruction(int n) {
    if (n == 0) return "Click to place the first vertex.";
    if (n == 1) return "Click to place more vertices (need 3+).";
    if (n == 2) return "Click to place more vertices (need 3+).";
    return "Click to add vertices.\nPress Enter or double-click to close.\nEsc to cancel.";
}

static const char* curve_2d_bezier_instruction(int n) {
    // Input order: P0, C0, C1, P1, C2, C3, P2, ...
    // n%3==0 → next is anchor; n%3==1 → ctrl1; n%3==2 → ctrl2
    // A complete segment exists when n>=4 and (n-1)%3==0
    if (n == 0) return "Click to place the start anchor point.";
    bool can_close = (n >= 4) && ((n - 1) % 3 == 0);
    int role = n % 3;
    if (role == 0) return "Click to place the next anchor point.";
    if (role == 2) return "Click to place control point 2.";
    // role == 1
    if (can_close)
        return "Segment complete! Press Enter to finish.\nOr click to add control point 1 of the next segment.\nEsc to cancel.";
    return "Click to place control point 1.";
}

static const char* curve_2d_bspline_instruction(int n) {
    // Input order: P0, P1, P2, ...
    // A complete segment exists when n>=4
    if (n == 0) return "Click to place the first control point.";
    if (n == 1) return "Click to place the second control point.";
    if (n == 2) return "Click to place the third control point.";
    if (n == 3) return "Click to place the fourth control point.";
    return "Click to add more control points.\nPress Enter or double-click to finish.\nEsc to cancel.";
}

static const char* surface_instruction() {
    // Bicubic surfaces are defined by an M×N control grid (M,N >= 4); clicking
    // can't place 3D control points, so use Create by Text.
    return "Bicubic surface: M×N control grid, M and N >= 4.\n"
           "Use the Create by Text modal; separate grid rows with ';'.\n"
           "Bezier needs M,N in {4,7,10,...}. Smoothness = samples per patch edge.";
}

// ─── DrawWindow ──────────────────────────────────────────────────────────────

void ObjectCreator::DrawWindow(){
    auto r = gui::layout::Get(gui::layout::Region::CreateObject);
    ImGui::SetNextWindowPos(r.pos, gui::layout::Cond());
    ImGui::SetNextWindowSize(r.size, gui::layout::Cond());
    ImGui::Begin("Create New Object");
        ImGui::Columns(2, "ObjectCreatorColumns", true);

        const bool is3d = AppConfig::is3d;

        // The available object set depends on 2D/3D mode. If the current mode is
        // no longer creatable here (e.g. Curve2D after switching to 3D), fall back
        // to Point so the UI never sits on a hidden selection.
        if (!core::typeAvailableInMode(mode, is3d)) {
            mode = core::ObjectType::POINT;
            points.clear();
        }

        // ── Mode radio buttons (filtered by 2D/3D mode) ──
        auto mode_radio = [&](const char* label, core::ObjectType t, int sub_method = -1) {
            bool selected = (mode == t) && (sub_method < 0 || method == sub_method);
            if (ImGui::RadioButton(label, selected)) {
                mode = t;
                if (sub_method >= 0) method = sub_method;
                points.clear();
                log.AddLog("Mode changed to %s\n", label);
            }
        };

        mode_radio("Point", core::ObjectType::POINT);     ImGui::SameLine();
        mode_radio("Line", core::ObjectType::LINE);       ImGui::SameLine();
        mode_radio("Wireframe", core::ObjectType::WIREFRAME); ImGui::SameLine();
        float polygon_x = ImGui::GetCursorPosX();
        mode_radio("Polygon", core::ObjectType::POLYGON);

        // ── Filled toggle (polygon only) ──
        if (mode == core::ObjectType::POLYGON) {
            ImGui::SetCursorPosX(polygon_x);
            ImGui::Checkbox("Filled", &filled);
        }

        float curve_x = ImGui::GetCursorPosX();
        if (!is3d) {
            // ── 2D-only: Curve2D ──
            mode_radio("2D Curve", core::ObjectType::CURVE2D);
            if (mode == core::ObjectType::CURVE2D) {
                ImGui::SetCursorPosX(curve_x);
                ImGui::RadioButton("Bezier", &method, 0); ImGui::SameLine();
                ImGui::RadioButton("B-Spline", &method, 1);

                ImGui::SetCursorPosX(curve_x);
                ImGui::Text("Smoothness:"); ImGui::SameLine();
                ImGui::PushItemWidth(80);
                ImGui::SliderInt("##smoothness", &curve_smoothness, 4, 300);
                ImGui::PopItemWidth();
            }
        } else {
            // ── 3D-only: bicubic surfaces ──
            mode_radio("3D Surface", core::ObjectType::SURFACE);
            if (mode == core::ObjectType::SURFACE) {
                ImGui::SetCursorPosX(curve_x);
                ImGui::RadioButton("Bezier", &method, 0); ImGui::SameLine();
                ImGui::RadioButton("B-Spline", &method, 1);

                // Tessellation technique — both produce identical geometry; the
                // choice selects which algorithm builds the surface.
                ImGui::SetCursorPosX(curve_x);
                ImGui::TextUnformatted("Eval:"); ImGui::SameLine();
                ImGui::RadioButton("Blending", &surf_eval, SURF_BLENDING); ImGui::SameLine();
                ImGui::RadioButton("Fwd Diff", &surf_eval, SURF_FORWARD_DIFF);

                ImGui::SetCursorPosX(curve_x);
                ImGui::Text("Smoothness:"); ImGui::SameLine();
                ImGui::PushItemWidth(80);
                ImGui::SliderInt("##surf_smoothness", &curve_smoothness, 4, 60);
                ImGui::PopItemWidth();
            }
        }


        // ── Object name ──
        ImGui::Text("Name (empty = auto):"); ImGui::SameLine();
        ImGui::InputText("##name", obj_name, IM_COUNTOF(obj_name));

        // ── Color picker ──
        ImGui::Text("Color:");  ImGui::SameLine();
        if (ImGui::ColorEdit3("##color", color_f)) {
            set_color(color_f[0], color_f[1], color_f[2]);
        }

        // ── Object creation by text ──
        // The text modal carries its own 2D/3D switch (seeded from the app mode),
        // so e.g. a Curve2D can still be created by text while the app is in 3D.
        if (ImGui::Button("Create by text...")) {
            text_creator.Open(mode, method, filled, is3d);
        }
        {
            std::vector<std::tuple<float, float, float>> text_pts;
            core::ObjectType text_mode   = mode;
            int             text_method = method;
            bool            text_filled = filled;
            int             text_rows = 0, text_cols = 0;
            if (text_creator.DrawModal(text_pts, text_mode, text_method, text_filled, text_rows, text_cols)) {
                points = text_pts;
                method = text_method;
                filled = text_filled;
                surf_rows = text_rows;
                surf_cols = text_cols;
                // Only adopt the modal's mode as the active radio when it matches
                // the current 2D/3D mode; otherwise create it without flipping UI.
                if (core::typeAvailableInMode(text_mode, is3d)) mode = text_mode;
                AddGraphicObjectAs(text_mode, text_method, text_filled);
            }
        }

        // ── Dynamic instructions ──
        ImGui::Spacing();
        ImGui::TextDisabled("=== Instructions ===");
        int n = (int)points.size();
        switch (mode) {
            case core::ObjectType::POINT:     ImGui::TextWrapped("%s", point_instruction()); break;
            case core::ObjectType::LINE:      ImGui::TextWrapped("%s", line_instruction(n)); break;
            case core::ObjectType::WIREFRAME: ImGui::TextWrapped("%s", wireframe_instruction(n)); break;
            case core::ObjectType::POLYGON:   ImGui::TextWrapped("%s", polygon_instruction(n)); break;
            case core::ObjectType::CURVE2D:
                if (method == BEZIER) ImGui::TextWrapped("%s", curve_2d_bezier_instruction(n));
                else                  ImGui::TextWrapped("%s", curve_2d_bspline_instruction(n));
                break;
            case core::ObjectType::SURFACE:   ImGui::TextWrapped("%s", surface_instruction()); break;
            default: break;
        }

        ImGui::NextColumn();

        // ── Import ──
        static char file_path[256] = "";
        ImGui::Text("Import from File:");
        ImGui::InputText("File Path", file_path, IM_COUNTOF(file_path));
        if(ImGui::Button("Import")){
            this->ImportFromFile(file_path);
        }

        // ── Export ──
        ImGui::Text("Export to File:");
        static char export_path[256] = "";
        ImGui::InputText("Export Path", export_path, IM_COUNTOF(export_path));
        if (ImGui::Button("Export")) {
            this->ExportToFile(export_path);
        }
    ImGui::End();

    // Push current in-progress state so Renderer can draw the creation preview.
    entityManager.setPreviewState(points, mode, method);
}

// ─── Input handling ──────────────────────────────────────────────────────────

void ObjectCreator::RegisterLeftClick(float x, float y, float z){
    points.push_back(std::make_tuple(x, y, z));
    if (mode == core::ObjectType::POINT ||
        (mode == core::ObjectType::LINE && points.size() == 2)) {
        AddGraphicObject();
    }
}

void ObjectCreator::CloseShape(){
    if (mode == core::ObjectType::POLYGON) {
        if (points.size() < 3) {
            log.AddLog("[error] Polygon needs at least 3 vertices.\n");
            return;
        }
        if (points.front() != points.back())
            points.push_back(points.front());
    } else if (mode == core::ObjectType::WIREFRAME) {
        if (points.size() < 2) {
            log.AddLog("[error] Wireframe needs at least 2 vertices.\n");
            return;
        }
    } else if (mode == core::ObjectType::CURVE2D){
        switch(method){
            case 0:{ // Bezier
                int n = (int)points.size();
                if (n < 4 || (n - 1) % 3 != 0) {
                    log.AddLog("[error] Bezier Curve needs 4, 7, 10, ... points (anchor, ctrl, ctrl, anchor, ...).\n");
                    return;
                }
                break;
            }
            case 1: {
                if (points.size() < 4) {
                    log.AddLog("[error] B-Spline Curve needs at least 4 control points.\n");
                    return;
                }
                break;
            }
            default: {
                log.AddLog("[error] Invalid curve method.\n");
                return;
            }
        }
        
    } else {
        return; // Point and Line auto-finish in RegisterLeftClick
    }
    AddGraphicObject();
}

void ObjectCreator::CancelCreation(){
    if (!points.empty()) {
        points.clear();
        log.AddLog("Object creation cancelled.\n");
    }
}

void ObjectCreator::AddGraphicObject(){
    AddGraphicObjectAs(mode, method, filled);
}

void ObjectCreator::AddGraphicObjectAs(core::ObjectType type, int curve_method, bool is_filled){
    std::string name(obj_name);
    ImU32 col = (ImU32)object_color;

    switch (type) {
        case core::ObjectType::POINT: {
            auto [x, y, z] = points[0];
            core::PointFactory f(name, x, y, z, col);
            entityManager.add(f);
            break;
        }
        case core::ObjectType::LINE: {
            core::LineFactory f(name, core::Point(points[0]), core::Point(points[1]), col);
            entityManager.add(f);
            break;
        }
        case core::ObjectType::WIREFRAME: {
            std::vector<core::Point> pts;
            pts.reserve(points.size());
            for (const auto& t : points) pts.emplace_back(t);
            core::WireframeFactory f(name, pts, col);
            entityManager.add(f);
            break;
        }
        case core::ObjectType::POLYGON: {
            std::vector<core::Point> pts;
            pts.reserve(points.size());
            for (const auto& t : points) pts.emplace_back(t);
            core::PolygonFactory f(name, pts, is_filled, col);
            entityManager.add(f);
            break;
        }
        case core::ObjectType::CURVE2D: {
            std::vector<core::Point> pts;
            pts.reserve(points.size());
            for (const auto& t : points) pts.emplace_back(t);
            core::Curve2DFactory f(name, pts, curve_smoothness, curve_method, col);
            entityManager.add(f);
            break;
        }
        case core::ObjectType::SURFACE: {
            // points carries the M×N control grid row-major (surf_rows × surf_cols),
            // validated by the text modal; the factory splits it into patches.
            if (surf_rows < 4 || surf_cols < 4 || (int)points.size() < surf_rows * surf_cols) {
                log.AddLog("[error] Surface needs an M×N control grid with M,N >= 4.\n");
                break;
            }
            std::vector<core::Point> grid;
            grid.reserve(points.size());
            for (const auto& t : points) grid.emplace_back(t);
            core::SurfaceFactory f(name, surf_rows, surf_cols, grid, curve_method, surf_eval, curve_smoothness, col);
            entityManager.add(f);
            break;
        }
        default: break;
    }
    points.clear();
}


// ─── File I/O ────────────────────────────────────────────────────────────────

void ObjectCreator::ImportFromFile(const char* file_path){
    ObjectIO::Import(file_path, entityManager, log);
}

void ObjectCreator::ExportToFile(const char* file_path){
    ObjectIO::Export(file_path, entityManager, log);
}
