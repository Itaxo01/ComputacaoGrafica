#include "ObjectCreator.hpp"
#include "imgui.h"
#include <fstream>
#include <sstream>

void ObjectCreator::DrawWindow(){
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 monitor_pos = viewport->Pos;
    ImVec2 monitor_size = viewport->Size;

    // Proportional window configurations based on the app window/monitor size
    ImGui::SetNextWindowPos(ImVec2(monitor_pos.x + monitor_size.x * (899.0f / 1700.0f), monitor_pos.y + monitor_size.y * (22.0f / 940.0f)), ImGuiCond_FirstUseEver); // Create New Object window position
    ImGui::SetNextWindowSize(ImVec2(monitor_size.x * (730.0f / 1700.0f), monitor_size.y * (204.0f / 940.0f)), ImGuiCond_FirstUseEver); // Create New Object window size
    ImGui::Begin("Create New Object");
        ImGui::Columns(2, "ObjectCreatorColumns", true);

        if (ImGui::Checkbox("Enable Object Creation:", &enable_object_creation)) {
            points.clear();
        }
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
        ImGui::Text("Object name:"); ImGui::SameLine();
        ImGui::InputText("##", obj_name, IM_COUNTOF(obj_name));
        
        ImGui::Text("Object color (RGB):"); ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        if (ImGui::DragInt3("##rgb", rgb_color, 1.0f, 0, 255)) {
            // Update the actual object color when changed, drag natively clamps but typed texts might need a check
            if (rgb_color[0] < 0) rgb_color[0] = 0; 
            if (rgb_color[0] > 255) rgb_color[0] = 255;
            if (rgb_color[1] < 0) rgb_color[1] = 0; 
            if (rgb_color[1] > 255) rgb_color[1] = 255;
            if (rgb_color[2] < 0) rgb_color[2] = 0; 
            if (rgb_color[2] > 255) rgb_color[2] = 255;
            
            set_color(rgb_color[0], rgb_color[1], rgb_color[2], 255);
        }

        ImGui::Text("To create a point: \nclick on canvas 1 time\n");
        ImGui::Text("To create a line: \nClick on canvas 2 times\n");
        ImGui::Text("To create a wireframe: \nClick on canvas at least 3 times and\nclick on the same location again.\n");

        ImGui::NextColumn();

        // Import from file
        static char file_path[256] = "";
        ImGui::Text("Import from File:");
        ImGui::InputText("File Path", file_path, IM_COUNTOF(file_path));
        if(ImGui::Button("Import")){
            this->ImportFromFile(file_path);
        }

        // Export to file
        ImGui::Text("Export to File:");
        static char export_path[256] = "";
        ImGui::InputText("Export Path", export_path, IM_COUNTOF(export_path));
        if (ImGui::Button("Export")) {
            this->ExportToFile(export_path);
        }
    ImGui::End();
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
            if (iss >> w) { } // successfully read w
            file_vertices.emplace_back(x, y, z); 
        } else if (type == "o" || type == "g") {
            iss >> current_name;
        } else if (type == "p") {
            points.clear();
            std::string v_str;
            while (iss >> v_str) {
                try {
                    int v_idx = std::stoi(v_str);
                    if (v_idx < 0) v_idx = file_vertices.size() + v_idx + 1; // Support relative negative indices
                    if (v_idx > 0 && v_idx <= file_vertices.size()) {
                        points.push_back(file_vertices[v_idx - 1]);
                    }
                } catch (...) {}
            }
            if (!points.empty()) {
                if (!current_name.empty()) entityManager.add(current_name, points, IM_COL32_WHITE);
                else entityManager.add(true, points, IM_COL32_WHITE);
                count++;
            }
        } else if (type == "l" || type == "f") {
            points.clear();
            std::string v_str;
            while (iss >> v_str) {
                try {
                    // std::stoi parses up to first non-digit, smartly ignoring /uv/normals (e.g. 1/2/3 -> 1)
                    int v_idx = std::stoi(v_str);
                    if (v_idx < 0) v_idx = file_vertices.size() + v_idx + 1;
                    if (v_idx > 0 && v_idx <= file_vertices.size()) {
                        points.push_back(file_vertices[v_idx - 1]);
                    }
                } catch (...) {}
            }
            if (!points.empty()) {
                // If it is a face, obj format intrinsically closes the loop. Re-add the first element if needed
                if (type == "f" && points.front() != points.back()) {
                    points.push_back(points.front());
                }
                if (!current_name.empty()) entityManager.add(current_name, points, IM_COL32_WHITE);
                else entityManager.add(true, points, IM_COL32_WHITE);
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
        
        // If wireframe naturally closes, write it as face 'f' instead of 'l'
        if (wireframe.points.size() > 2 && wireframe.points.front() == wireframe.points.back()) {
            file << "f";
            // Ignore the duplicated back vertex since face representation is intrinsically closed
            for (size_t i = 0; i < wireframe.points.size() - 1; ++i) {
                file << " " << vertex_index + i;
            }
        } else {
            file << "l";
            for (size_t i = 0; i < wireframe.points.size(); ++i) {
                file << " " << vertex_index + i;
            }
        }
        file << "\n";
        vertex_index += wireframe.points.size();
    }
    
    log.AddLog("Exported objects to %s\n", path.c_str());
}


void ObjectCreator::RegisterLeftClick(float x, float y, float z){
    if (enable_object_creation) {
        auto &[x1, y1, z1] = points.back();
        if (points.size() > 2 && mode == core::ShapeType::WIREFRAME &&
            x == x1 && y == y1 && z == z1) {
            AddGraphicObject();
            return;
        }

        points.push_back(std::make_tuple(x, y, z));
        if (mode == core::ShapeType::POINT || (mode == core::ShapeType::LINE && points.size() == 2)) {
            AddGraphicObject();
        }
    }
} 

void ObjectCreator::AddGraphicObject(){
    std::string name(obj_name);
    if (name == "" || name == "\1") { // Substituir por um regex depois...
        log.AddLog("[error] Cannot create object (Invalid name): {%s}\n", obj_name);
        return;
    } else if (name == "DEFAULT_NAME"){
        log.AddLog("Creating new object... auto-generated name\n");
        entityManager.add(true, points, object_color);
        points.clear();
    } else {
        log.AddLog("Creating new object... name: {%s}\n", obj_name);
        entityManager.add(name, points, object_color);
        points.clear();
    }
}