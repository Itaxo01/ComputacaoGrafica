#include "ObjectCreator.hpp"
#include "ObjectIO.hpp"
#include "imgui.h"
#include "ObjectFactories/PointFactory.hpp"
#include "ObjectFactories/LineFactory.hpp"
#include "ObjectFactories/WireframeFactory.hpp"
#include "ObjectFactories/PolygonFactory.hpp"
#include "ObjectFactories/Curve2DFactory.hpp"
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

// ─── DrawWindow ──────────────────────────────────────────────────────────────

void ObjectCreator::DrawWindow(){
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 monitor_pos = viewport->Pos;
    ImVec2 monitor_size = viewport->Size;

    ImGui::SetNextWindowPos(ImVec2(monitor_pos.x + monitor_size.x * (899.0f / 1700.0f), monitor_pos.y + monitor_size.y * (22.0f / 940.0f)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(monitor_size.x * (730.0f / 1700.0f), monitor_size.y * (204.0f / 940.0f)), ImGuiCond_FirstUseEver);
    ImGui::Begin("Create New Object");
        ImGui::Columns(2, "ObjectCreatorColumns", true);

        // ── Mode radio buttons ──
        if (ImGui::RadioButton("Point", &e, 0)) {
            log.AddLog("Mode changed to POINT\n");
            mode = core::ObjectType::POINT;
            points.clear();
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Line", &e, 1)) {
            log.AddLog("Mode changed to LINE\n");
            mode = core::ObjectType::LINE;
            points.clear();
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Wireframe", &e, 2)) {
            log.AddLog("Mode changed to WIREFRAME\n");
            mode = core::ObjectType::WIREFRAME;
            points.clear();
        }
        ImGui::SameLine();
        float polygon_x = ImGui::GetCursorPosX();
        if (ImGui::RadioButton("Polygon", &e, 3)) {
            log.AddLog("Mode changed to POLYGON\n");
            mode = core::ObjectType::POLYGON;
            points.clear();
        }

        // ── Filled toggle (polygon only) ──
        if (mode == core::ObjectType::POLYGON) {
            ImGui::SetCursorPosX(polygon_x);
            ImGui::Checkbox("Filled", &filled);
        }

        float curve_x = ImGui::GetCursorPosX();
        if (ImGui::RadioButton("2D Curve", &e, 4)) {
            log.AddLog("Mode changed to CURVE2D\n");
            mode = core::ObjectType::CURVE2D;
            points.clear();
        }

        // ── Smoothness (curve only) ──
        if (mode == core::ObjectType::CURVE2D){
            ImGui::SetCursorPosX(curve_x);
            ImGui::RadioButton("Bezier", &method, 0); ImGui::SameLine();
            ImGui::RadioButton("B-Spline", &method, 1);
        }

        if (mode == core::ObjectType::CURVE2D) {
            ImGui::SetCursorPosX(curve_x);
            ImGui::Text("Smoothness:"); ImGui::SameLine();
            ImGui::PushItemWidth(80);
            ImGui::SliderInt("##smoothness", &curve_smoothness, 4, 300);
            ImGui::PopItemWidth();
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
        if (ImGui::Button("Create by text...")) {
            text_creator.Open(mode, method, filled);
        }
        {
            std::vector<std::tuple<float, float, float>> text_pts;
            core::ObjectType text_mode   = mode;
            int             text_method = method;
            bool            text_filled = filled;
            if (text_creator.DrawModal(text_pts, text_mode, text_method, text_filled)) {
                points = text_pts;
                mode   = text_mode;
                method = text_method;
                filled = text_filled;
                // sync radio index
                switch (mode) {
                    case core::ObjectType::LINE:      e = 1; break;
                    case core::ObjectType::WIREFRAME: e = 2; break;
                    case core::ObjectType::POLYGON:   e = 3; break;
                    case core::ObjectType::CURVE2D:   e = 4; break;
                    default:                         e = 0; break;
                }
                AddGraphicObject();
            }
        }

        // ── Dynamic instructions ──
        ImGui::Spacing();
        ImGui::TextDisabled("=== Instructions ===");
        int n = (int)points.size();
        switch (e) {
            case 0: ImGui::TextWrapped("%s", point_instruction()); break;
            case 1: ImGui::TextWrapped("%s", line_instruction(n)); break;
            case 2: ImGui::TextWrapped("%s", wireframe_instruction(n)); break;
            case 3: ImGui::TextWrapped("%s", polygon_instruction(n)); break;
            case 4: {
                if(method == 0) ImGui::TextWrapped("%s", curve_2d_bezier_instruction(n));
                else ImGui::TextWrapped("%s", curve_2d_bspline_instruction(n));
                break;
            }
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
    std::string name(obj_name);
    ImU32 col = (ImU32)object_color;

    switch (mode) {
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
            core::PolygonFactory f(name, pts, filled, col);
            entityManager.add(f);
            break;
        }
        case core::ObjectType::CURVE2D: {
            std::vector<core::Point> pts;
            pts.reserve(points.size());
            for (const auto& t : points) pts.emplace_back(t);
            core::Curve2DFactory f(name, pts, curve_smoothness, method, col);
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
