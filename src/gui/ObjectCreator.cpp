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

// ─── File I/O (unchanged) ────────────────────────────────────────────────────

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

    std::string line;
    unsigned int count = 0;
    std::vector<std::tuple<float, float, float>> file_vertices;
    std::string current_name;

    while(std::getline(file, line)){
        if(line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z, w = 1.0;
            iss >> x >> y >> z;
            if (iss >> w) { }
            file_vertices.emplace_back(x, y, z);
        } else if (type == "o" || type == "g") {
            iss >> current_name;
        } else if (type == "p") {
            points.clear();
            std::string v_str;
            while (iss >> v_str) {
                try {
                    int v_idx = std::stoi(v_str);
                    if (v_idx < 0) v_idx = file_vertices.size() + v_idx + 1;
                    if (v_idx > 0 && v_idx <= (int)file_vertices.size()) {
                        points.push_back(file_vertices[v_idx - 1]);
                    }
                } catch (...) {}
            }
            if (!points.empty()) {
                /*if (!current_name.empty()) entityManager.add(current_name, points, IM_COL32_WHITE);
                else entityManager.add(true, points, IM_COL32_WHITE);*/
                AddGraphicObject();
                count++;
            }
        } else if (type == "l" || type == "f") {
            points.clear();
            std::string v_str;
            while (iss >> v_str) {
                try {
                    int v_idx = std::stoi(v_str);
                    if (v_idx < 0) v_idx = file_vertices.size() + v_idx + 1;
                    if (v_idx > 0 && v_idx <= (int)file_vertices.size()) {
                        points.push_back(file_vertices[v_idx - 1]);
                    }
                } catch (...) {}
            }
            if (!points.empty()) {
                if (type == "f" && points.front() != points.back()) {
                    points.push_back(points.front());
                }
                /*if (!current_name.empty()) entityManager.add(current_name, points, IM_COL32_WHITE);
                else entityManager.add(true, points, IM_COL32_WHITE);*/
                AddGraphicObject();
                count++;
            }
        }
    }
    points.clear();
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
        file << "o " << point.getName() << "\n";
        file << "v " << point.x << " " << point.y << " 0.0\n";
        file << "p " << vertex_index << "\n";
        vertex_index++;
    }
    for (const auto &line: entityManager.getLineList()){
        file << "o " << line.getName() << "\n";
        file << "v " << line.a.x << " " << line.a.y << " 0.0\n";
        file << "v " << line.b.x << " " << line.b.y << " 0.0\n";
        file << "l " << vertex_index << " " << vertex_index+1 << "\n";
        vertex_index += 2;
    }
    for (const auto& wireframe : entityManager.getWireframeList()) {
        file << "o " << wireframe.getName() << "\n";
        for (const auto& vertex : wireframe.points) {
            file << "v " << vertex.x << " " << vertex.y << " 0.0\n";
        }
        if (wireframe.points.size() > 2 && wireframe.points.front() == wireframe.points.back()) {
            file << "f";
            for (size_t i = 0; i < wireframe.points.size() - 1; ++i)
                file << " " << vertex_index + i;
        } else {
            file << "l";
            for (size_t i = 0; i < wireframe.points.size(); ++i)
                file << " " << vertex_index + i;
        }
        file << "\n";
        vertex_index += wireframe.points.size();
    }

    log.AddLog("Exported objects to %s\n", path.c_str());
}
