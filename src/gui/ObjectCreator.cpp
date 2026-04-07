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
    
    unsigned int count = 0;
    while(std::getline(file, line)){
        if(line.empty()) continue;

        const char*ptr = line.c_str();
        char *next_ptr = nullptr; 

        if(line.compare(0, 5, "POINT") == 0) {
            ptr += 5;
            float x = std::strtof(ptr, &next_ptr); ptr = next_ptr;
            float y = std::strtof(ptr, &next_ptr);

            points.clear();
            points.emplace_back(x, y);
            entityManager.add(true, points);
            count++;
        } else if(line.compare(0, 4, "LINE") == 0){
            ptr += 4;
            float x1 = std::strtof(ptr, &next_ptr); ptr = next_ptr;
            float y1 = std::strtof(ptr, &next_ptr); ptr = next_ptr;
            float x2 = std::strtof(ptr, &next_ptr); ptr = next_ptr;
            float y2 = std::strtof(ptr, &next_ptr);
            points.clear();
            points.emplace_back(x1, y1);
            points.emplace_back(x2, y2);
            entityManager.add(true, points);
            count++;
        } else if(line.compare(0, 9, "WIREFRAME") == 0){
            ptr += 9;
            points.clear();
            // strtof sets next_ptr to ptr if it finds no more numbers
            while (true) {
                float x = std::strtof(ptr, &next_ptr);
                if (ptr == next_ptr) break; // No more numbers on this line
                ptr = next_ptr;
                
                float y = std::strtof(ptr, &next_ptr);
                ptr = next_ptr;
                
                points.emplace_back(x, y);
            }
            if (!points.empty()) {
                entityManager.add(true, points);
                count++;
            }
        }
        points.clear();
    }
    log.AddLog("Imported %d objects from %s\n", count, file_path);
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
        for (const auto& vertex : wireframe.points) {
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