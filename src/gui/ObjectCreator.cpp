#include "ObjectCreator.hpp"
#include "ObjectIO.hpp"
#include "Shape.hpp"
#include "imgui.h"
#include <fstream>
#include <sstream>

// ─── Dynamic instruction text ────────────────────────────────────────────────

static const char* point_instruction() {
    return "Click on the canvas to place a point.";
}

static const char* line_instruction(int n) {
    if (n == 0) return "Click to place the first endpoint.";
    return "Click to place the second endpoint.";
}

static const char* wireframe_instruction(int n) {
    if (n == 0) return "Click to place the first vertex.";
    if (n == 1) return "Click to place more vertices.\nPress Enter or double-click to finish.";
    return "Click to add vertices.\nPress Enter or double-click to finish.\nEsc to cancel.";
}

static const char* polygon_instruction(int n) {
    if (n == 0) return "Click to place the first vertex.";
    if (n == 1) return "Click to place more vertices (need 3+).";
    if (n == 2) return "Click to place more vertices (need 3+).";
    return "Click to add vertices.\nPress Enter or double-click to close.\nEsc to cancel.";
}

static const char* bezier_curve_instruction(int n) {
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
            mode = core::ShapeType::POINT;
            points.clear();
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Line", &e, 1)) {
            log.AddLog("Mode changed to LINE\n");
            mode = core::ShapeType::LINE;
            points.clear();
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Wireframe", &e, 2)) {
            log.AddLog("Mode changed to WIREFRAME\n");
            mode = core::ShapeType::WIREFRAME;
            points.clear();
        }
        ImGui::SameLine();
        float polygon_x = ImGui::GetCursorPosX();
        if (ImGui::RadioButton("Polygon", &e, 3)) {
            log.AddLog("Mode changed to POLYGON\n");
            mode = core::ShapeType::POLYGON;
            points.clear();
        }

        // ── Filled toggle (polygon only) ──
        if (mode == core::ShapeType::POLYGON) {
            ImGui::SetCursorPosX(polygon_x);
            ImGui::Checkbox("Filled", &filled);
        }

        if (ImGui::RadioButton("Bezier Curve", &e, 4)) {
            log.AddLog("Mode changed to BEZIER_CURVE\n");
            mode = core::ShapeType::BEZIER_CURVE;
            points.clear();
        }

        // ── Object name ──
        ImGui::Text("Name (empty = auto):"); ImGui::SameLine();
        ImGui::InputText("##name", obj_name, IM_COUNTOF(obj_name));

        // ── Color picker ──
        ImGui::Text("Color:");  ImGui::SameLine();
        if (ImGui::ColorEdit3("##color", color_f)) {
            set_color(color_f[0], color_f[1], color_f[2]);
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
            case 4: ImGui::TextWrapped("%s", bezier_curve_instruction(n)); break;
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
    entityManager.setPreviewState(points, mode);
}

// ─── Input handling ──────────────────────────────────────────────────────────

void ObjectCreator::RegisterLeftClick(float x, float y, float z){
    points.push_back(std::make_tuple(x, y, z));
    if (mode == core::ShapeType::POINT ||
        (mode == core::ShapeType::LINE && points.size() == 2)) {
        AddGraphicObject();
    }
}

void ObjectCreator::CloseShape(){
    if (mode == core::ShapeType::POLYGON) {
        if (points.size() < 3) {
            log.AddLog("[error] Polygon needs at least 3 vertices.\n");
            return;
        }
        if (points.front() != points.back())
            points.push_back(points.front());
    } else if (mode == core::ShapeType::WIREFRAME) {
        if (points.size() < 2) {
            log.AddLog("[error] Wireframe needs at least 2 vertices.\n");
            return;
        }
    } else if (mode == core::ShapeType::BEZIER_CURVE){
        int n = (int)points.size();
        if (n < 4 || (n - 1) % 3 != 0) {
            log.AddLog("[error] Bezier curve needs 4, 7, 10, ... points (anchor, ctrl, ctrl, anchor, ...).\n");
            return;
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
    bool auto_name = name.empty();

    if (auto_name) entityManager.add(true, points, mode, filled,  object_color);
    else           entityManager.add(name, points, mode, filled, object_color);
    points.clear();
}

// ─── File I/O ────────────────────────────────────────────────────────────────

void ObjectCreator::ImportFromFile(const char* file_path){
    std::string path(file_path);
    if (path.length() < 4 || path.substr(path.length() - 4) != ".obj")
        path += ".obj";

    auto validation = ValidateObjFile(path);
    if (!validation.valid) {
        log.AddLog("[error] %s\n", validation.error.c_str());
        return;
    }
    log.AddLog("[info] Validation passed: %d vertices, %d objects, %d colored.\n",
               validation.vertex_count, validation.object_count, validation.color_count);

    std::ifstream file(path);
    if (!file.is_open()) {
        log.AddLog("[error] Failed to open file: %s\n", path.c_str());
        return;
    }

    std::string line;
    unsigned int count = 0;
    std::vector<std::tuple<float, float, float>> file_vertices;
    std::string current_name;
    int  pending_color  = IM_COL32_WHITE;
    bool pending_filled = false;
    bool pending_bezier = false;

    auto resolve_indices = [&](std::istringstream &iss) {
        RawPts pts;
        std::string v_str;
        while (iss >> v_str) {
            try {
                int idx = std::stoi(v_str);
                if (idx < 0) idx = (int)file_vertices.size() + idx + 1;
                if (idx > 0 && idx <= (int)file_vertices.size())
                    pts.push_back(file_vertices[idx - 1]);
            } catch (...) {}
        }
        return pts;
    };

    while (std::getline(file, line)) {
        if (!line.empty() && line[0] == '#') {
            std::istringstream css(line.substr(1));
            std::string tag;
            css >> tag;
            if (tag == "color") {
                int r, g, b, a = 255;
                if (css >> r >> g >> b) { css >> a; }
                pending_color = IM_COL32(r, g, b, a);
            } else if (tag == "filled") {
                int f = 0; css >> f; pending_filled = (f != 0);
            } else if (tag == "type") {
                std::string t; css >> t; pending_bezier = (t == "bezier_curve");
            }
            continue;
        }
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z = 0.0f;
            iss >> x >> y >> z;
            file_vertices.emplace_back(x, y, z);
        } else if (type == "o" || type == "g") {
            iss >> current_name;
            pending_color  = IM_COL32_WHITE;
            pending_filled = false;
            pending_bezier = false;
        } else if (type == "p") {
            auto pts = resolve_indices(iss);
            ImportPoint(current_name, pts, pending_color, entityManager);
            if (!pts.empty()) count++;
        } else if (type == "l") {
            auto pts = resolve_indices(iss);
            if (pending_bezier) {
                ImportBezierCurve(current_name, pts, pending_color, entityManager);
                if ((int)pts.size() >= 4 && ((int)pts.size() - 1) % 3 == 0) count++;
            } else {
                ImportWireframe(current_name, pts, pending_color, entityManager);
                if (pts.size() >= 2) count++;
            }
        } else if (type == "f") {
            auto pts = resolve_indices(iss);
            ImportPolygon(current_name, pts, pending_color, pending_filled, entityManager);
            if (pts.size() >= 3) count++;
        }
    }

    log.AddLog("Imported %d objects from %s\n", count, path.c_str());
}

void ObjectCreator::ExportToFile(const char* file_path){
    std::string path(file_path);
    if (path.length() < 4 || path.substr(path.length() - 4) != ".obj")
        path += ".obj";

    std::ofstream file(path);
    if (!file.is_open()) {
        log.AddLog("[error] Failed to open file: %s\n", path.c_str());
        return;
    }

    int vi = 1;
    file << "# Fast OBJ Export\n";
    ExportPoints      (file, entityManager.getPointList(),       vi);
    ExportLines       (file, entityManager.getLineList(),        vi);
    ExportWireframes  (file, entityManager.getWireframeList(),   vi);
    ExportPolygons    (file, entityManager.getPolygonList(),     vi);
    ExportBezierCurves(file, entityManager.getBezierCurveList(), vi);

    log.AddLog("Exported objects to %s\n", path.c_str());
}
