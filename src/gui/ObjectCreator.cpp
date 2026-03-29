#include "ObjectCreator.hpp"
#include "imgui.h"
#include <fstream>
#include <sstream>

void ObjectCreator::DrawWindow(){
    ImGui::SetNextWindowPos(ImVec2(876, 26), ImGuiCond_FirstUseEver); // Create New Object window position
    ImGui::SetNextWindowSize(ImVec2(784, 235), ImGuiCond_FirstUseEver); // Create New Object window size
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
    std::ifstream file(file_path);
    if (!file.is_open()) {
        log.AddLog("[error] Failed to open file: %s\n", file_path);
        return;
    }
    std::string line;
    while(std::getline(file, line)){
        std::istringstream iss(line);
        std::string type;
        iss>>type;
        if(type == "POINT"){
            float x, y;
            iss>>x>>y;
            points.clear();
            points.emplace_back(x, y);
            AddGraphicObject();
        }else if (type == "LINE") {
            float x1, y1, x2, y2;
            iss >> x1 >> y1 >> x2 >> y2;
            points.clear();
            points.emplace_back(x1, y1);
            points.emplace_back(x2, y2);
            AddGraphicObject();
        } else if (type == "WIREFRAME") {
            points.clear();
            float x, y;
            while (iss >> x >> y) {
                points.emplace_back(x, y);
            }
            AddGraphicObject();
        } else {
            log.AddLog("[error] Unknown object type in file: %s\n", type.c_str());
        }
    }
}


void ObjectCreator::ExportToFile(const char* file_path){
    std::ofstream file(file_path);
    if (!file.is_open()) {
        log.AddLog("[error] Failed to open file: %s\n", file_path);
        return;
    }
    for (const auto &point: entityManager.getPointList()){
        file << "POINT "<<point.x<<" "<<point.y<<'\n';
    }
    for (const auto &line: entityManager.getLineList()){
        file << "LINE "<<line.a.x<<" "<<line.a.y<<" "<<line.b.x<<" "<<line.b.y<<'\n';
    }
    for (const auto& wireframe : entityManager.getWireframeList()) {
        file << "WIREFRAME";
        for (const auto& vertex : wireframe.data) {
            file << " " << vertex.x << " " << vertex.y;
        }
        file << "\n";
    }

}


void ObjectCreator::RegisterLeftClick(float x, float y){
    if (enable_object_creation) {
        if (points.size() > 2 && mode == core::ShapeType::WIREFRAME &&
            x == points.back().first && y == points.back().second) {
            AddGraphicObject();
            return;
        }

        points.push_back(std::make_pair(x, y));
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
        entityManager.add(true, points);
        points.clear();
    } else {
        log.AddLog("Creating new object... name: {%s}\n", obj_name);
        entityManager.add(name, points);
        points.clear();
    }
}