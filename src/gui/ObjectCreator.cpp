#include "ObjectCreator.hpp"
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
    if (n == 0) return "Click to place the first vertex.";
    if (n == 1) return "Click to place more vertices (need 1+).";
    return "Click to add control vertices.\nPress Enter or double-click to close.\nEsc to cancel.";
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
        if (points.size() < 2) {
            log.AddLog("[error] Bezier curve needs at least 2 vertices.\n");
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

// Extract R,G,B,A (0-255) from an ImU32 color (layout: A<<24|B<<16|G<<8|R).
static void unpack_color(int col, int &r, int &g, int &b, int &a) {
    unsigned int uc = (unsigned int)col;
    r = (uc >>  0) & 0xFF;
    g = (uc >>  8) & 0xFF;
    b = (uc >> 16) & 0xFF;
    a = (uc >> 24) & 0xFF;
}

void ObjectCreator::ImportFromFile(const char* file_path){
    std::string path(file_path);
    if (path.length() < 4 || path.substr(path.length() - 4) != ".obj") {
        path += ".obj";
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        log.AddLog("[error] Failed to open file: %s. The suffix of the path should include .obj\n", path.c_str());
        return;
    }

    // Salva os dados atuais para restaurar depois da importação
    core::ShapeType saved_mode   = mode;
    bool            saved_filled = filled;
    int             saved_color  = object_color;

    std::string line;
    unsigned int count = 0;
    std::vector<std::tuple<float, float, float>> file_vertices;
    std::string current_name;

    // Per-object metadata parsed from custom comments
    int  pending_color  = IM_COL32_WHITE;
    bool pending_filled = false;

    while(std::getline(file, line)){
        // Handle custom metadata comments before skipping all '#' lines
        if (!line.empty() && line[0] == '#') {
            std::istringstream css(line.substr(1));
            std::string tag;
            css >> tag;
            if (tag == "color") {
                int r, g, b, a = 255;
                if (css >> r >> g >> b) {
                    css >> a;
                    pending_color = IM_COL32(r, g, b, a);
                }
            } else if (tag == "filled") {
                int f = 0;
                css >> f;
                pending_filled = (f != 0);
            }
            continue;
        }
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z = 0.0f, w = 1.0f;
            iss >> x >> y >> z;
            (void)w;
            file_vertices.emplace_back(x, y, z);
        } else if (type == "o" || type == "g") {
            iss >> current_name;
            // Reset per-object metadata for each new object
            pending_color  = IM_COL32_WHITE;
            pending_filled = false;
        } else if (type == "p") {
            points.clear();
            std::string v_str;
            while (iss >> v_str) {
                try {
                    int v_idx = std::stoi(v_str);
                    if (v_idx < 0) v_idx = (int)file_vertices.size() + v_idx + 1;
                    if (v_idx > 0 && v_idx <= (int)file_vertices.size())
                        points.push_back(file_vertices[v_idx - 1]);
                } catch (...) {}
            }
            if (!points.empty()) {
                mode         = core::ShapeType::POINT;
                object_color = pending_color;
                filled       = false;
                if (!current_name.empty()) {
                    std::strncpy(obj_name, current_name.c_str(), sizeof(obj_name) - 1);
                    obj_name[sizeof(obj_name) - 1] = '\0';
                } else {
                    obj_name[0] = '\0';
                }
                AddGraphicObject();
                count++;
            }
        } else if (type == "l") {
            // 'l' → Wireframe (open or closed polyline)
            points.clear();
            std::string v_str;
            while (iss >> v_str) {
                try {
                    int v_idx = std::stoi(v_str);
                    if (v_idx < 0) v_idx = (int)file_vertices.size() + v_idx + 1;
                    if (v_idx > 0 && v_idx <= (int)file_vertices.size())
                        points.push_back(file_vertices[v_idx - 1]);
                } catch (...) {}
            }
            if (points.size() >= 2) {
                mode         = core::ShapeType::WIREFRAME;
                object_color = pending_color;
                filled       = false;
                if (!current_name.empty()) {
                    std::strncpy(obj_name, current_name.c_str(), sizeof(obj_name) - 1);
                    obj_name[sizeof(obj_name) - 1] = '\0';
                } else {
                    obj_name[0] = '\0';
                }
                AddGraphicObject();
                count++;
            }
        } else if (type == "f") {
            // 'f' → Polygon (closed; filled flag from # filled comment)
            points.clear();
            std::string v_str;
            while (iss >> v_str) {
                try {
                    // OBJ faces can be "v/vt/vn" — take only the vertex index part
                    int v_idx = std::stoi(v_str);
                    if (v_idx < 0) v_idx = (int)file_vertices.size() + v_idx + 1;
                    if (v_idx > 0 && v_idx <= (int)file_vertices.size())
                        points.push_back(file_vertices[v_idx - 1]);
                } catch (...) {}
            }
            if (points.size() >= 3) {
                mode         = core::ShapeType::POLYGON;
                filled       = pending_filled;
                object_color = pending_color;
                if (!current_name.empty()) {
                    std::strncpy(obj_name, current_name.c_str(), sizeof(obj_name) - 1);
                    obj_name[sizeof(obj_name) - 1] = '\0';
                } else {
                    obj_name[0] = '\0';
                }
                AddGraphicObject();
                count++;
            }
        }
    }
    points.clear();
    obj_name[0] = '\0';

    // Restore UI state
    mode         = saved_mode;
    filled       = saved_filled;
    object_color = saved_color;

    log.AddLog("Imported %d objects from %s\n", count, path.c_str());
}


void ObjectCreator::ExportToFile(const char* file_path){
    std::string path(file_path);
    if (path.length() < 4 || path.substr(path.length() - 4) != ".obj") {
        path += ".obj";
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        log.AddLog("[error] Failed to open file: %s\n", path.c_str());
        return;
    }

    int vertex_index = 1;
    file << "# Fast OBJ Export\n";

    for (const auto &point: entityManager.getPointList()){
        int r, g, b, a;
        unpack_color(point.object_color, r, g, b, a);
        file << "o " << point.getName() << "\n";
        file << "# color " << r << " " << g << " " << b << " " << a << "\n";
        file << "v " << point.x << " " << point.y << " 0.0\n";
        file << "p " << vertex_index << "\n";
        vertex_index++;
    }

    for (const auto &ln: entityManager.getLineList()){
        int r, g, b, a;
        unpack_color(ln.object_color, r, g, b, a);
        file << "o " << ln.getName() << "\n";
        file << "# color " << r << " " << g << " " << b << " " << a << "\n";
        file << "v " << ln.a.x << " " << ln.a.y << " 0.0\n";
        file << "v " << ln.b.x << " " << ln.b.y << " 0.0\n";
        file << "l " << vertex_index << " " << vertex_index + 1 << "\n";
        vertex_index += 2;
    }

    for (const auto& wireframe : entityManager.getWireframeList()) {
        int r, g, b, a;
        unpack_color(wireframe.object_color, r, g, b, a);
        file << "o " << wireframe.getName() << "\n";
        file << "# color " << r << " " << g << " " << b << " " << a << "\n";
        for (const auto& vertex : wireframe.points) {
            file << "v " << vertex.x << " " << vertex.y << " 0.0\n";
        }
        file << "l";
        for (size_t i = 0; i < wireframe.points.size(); ++i)
            file << " " << vertex_index + i;
        file << "\n";
        vertex_index += (int)wireframe.points.size();
    }

    for (const auto& polygon : entityManager.getPolygonList()) {
        int r, g, b, a;
        unpack_color(polygon.object_color, r, g, b, a);
        file << "o " << polygon.getName() << "\n";
        file << "# color " << r << " " << g << " " << b << " " << a << "\n";
        file << "# filled " << (polygon.filled ? 1 : 0) << "\n";
        // Polygons store a closing duplicate of the first vertex — skip it on export
        size_t n = polygon.points.size();
        if (n > 1 && polygon.points.front().x == polygon.points.back().x &&
                     polygon.points.front().y == polygon.points.back().y)
            n--;
        for (size_t i = 0; i < n; ++i)
            file << "v " << polygon.points[i].x << " " << polygon.points[i].y << " 0.0\n";
        file << "f";
        for (size_t i = 0; i < n; ++i)
            file << " " << vertex_index + i;
        file << "\n";
        vertex_index += (int)n;
    }

    log.AddLog("Exported objects to %s\n", path.c_str());
}
