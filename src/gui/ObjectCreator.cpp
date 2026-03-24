#include "ObjectCreator.hpp"

void ObjectCreator::DrawWindow(){
    ImGui::SetNextWindowPos(ImVec2(869, 27), ImGuiCond_FirstUseEver); // Create New Object window position
    ImGui::SetNextWindowSize(ImVec2(366, 232), ImGuiCond_FirstUseEver); // Create New Object window size
    ImGui::Begin("Create New Object");
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
    ImGui::End();
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
    }
    log.AddLog("Creating new object... name: {%s}\n", obj_name);
    entityManager.add(name, points);
    points.clear();
}