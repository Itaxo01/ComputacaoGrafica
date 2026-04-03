#include "ObjectListener.hpp"
#include <string>
#include <algorithm>

#define DFM_INPUT_BOX_SIZE 100
#define DFM_BUTTON_SIZE ImVec2(50.0f, 20.0f)
#define DEFAULT_SPLIT_CHAR '|'

const char* ObjectListener::GetTypeName(core::ShapeType type) {
    switch (type) {
        case core::ShapeType::POINT: return "Point";
        case core::ShapeType::LINE: return "Line";
        case core::ShapeType::WIREFRAME: return "Wireframe";
        default: return "Unknown";
    }
}

void ObjectListener::HandleAddScaling(float x, float y) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Scaling: (%.1f, %.1f)", x, y);
    transform_buf_names.push_back(buffer);
    // TODO
}

void ObjectListener::HandleAddTranslation(float x, float y) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Translation: (%.1f, %.1f)", x, y);
    transform_buf_names.push_back(buffer);
    // TODO
}

void ObjectListener::HandleAddRotation(float x, float y, float angle) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Rotation: {(%.1f, %.1f), %.1f}", x, y, angle);
    transform_buf_names.push_back(buffer);
    // TODO
    ;
}

void ObjectListener::DrawObjectList() {
    // Todo display abaixo será substituído pelo MultipleSelectionList.
    // A lógica será transferida para o Controller
    // BEGIN
    const auto& manifest = entityManager.GetManifest();
    int total_items = manifest.size();
    int total_pages = (total_items + items_per_page - 1) / items_per_page; // arendonda para cima.

    // Diminui automaticamente no caso de deleção de algum elemento.
    if (current_page >= total_pages && total_pages > 0){
        current_page = total_pages - 1;
    }

    ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);

    // Controles da paginação
    ImGui::PushItemWidth(100);
    if (ImGui::Button("<") && current_page > 0) {
        current_page--;
    }
    ImGui::SameLine();
    ImGui::Text("%d / %d", current_page + 1, std::max(1, total_pages));
    ImGui::SameLine();
    if (ImGui::Button(">") && current_page < total_pages - 1) {
        current_page++;
    }
    ImGui::Separator();

    int start_idx = current_page * items_per_page;
    int end_idx = std::min(start_idx + items_per_page, total_items);

    for (int i = start_idx; i < end_idx; ++i) {
        const auto& entry = manifest[i];
        
        // Check if current item is in our selected set
        bool is_selected = selected_ids.find(entry.id) != selected_ids.end();
        
        // Format the display label
        std::string label = "[" + std::to_string(entry.fake_id) + "] " + entry.name + " (" + GetTypeName(entry.type) + ")";

        // Draw the selectable item
        if (ImGui::Selectable(label.c_str(), is_selected)) {
            ImGuiIO& io = ImGui::GetIO();
            
            if (io.KeyCtrl) {
                // CTRL+Click: Toggle selection
                if (is_selected) selected_ids.erase(entry.id);
                else selected_ids.insert(entry.id);
                last_selected_index = i;
            } 
            else if (io.KeyShift && last_selected_index != -1) {
                // SHIFT+Click: Select range
                selected_ids.clear();
                int start = std::min(last_selected_index, i);
                int end = std::max(last_selected_index, i);
                for (int j = start; j <= end; ++j) {
                    selected_ids.insert(manifest[j].id);
                }
            } 
            else {
                // Normal Click: Clear others, select this one
                selected_ids.clear();
                selected_ids.insert(entry.id);
                last_selected_index = i;
            }
        }

        // --- CONTEXT MENU (Right Click) ---
        // We tie the context menu to the item. It opens if you right click a hovered item.
        if (ImGui::BeginPopupContextItem(("context_menu_" + std::to_string(entry.id)).c_str())){
            
            // If the user right-clicks an unselected item, select ONLY that item
            if (!is_selected) {
                selected_ids.clear();
                selected_ids.insert(entry.id);
                last_selected_index = i;
            }

            ImGui::Text("Operations (%zu selected)", selected_ids.size());
            ImGui::Separator();

            if (ImGui::MenuItem("Delete")) {
                for (long long id : selected_ids) {
                    entityManager.remove(id); // Implement this on EntityManager/DisplayFile
                }
                selected_ids.clear();
                last_selected_index = -1;
            }
            
            if (ImGui::MenuItem("Rotate (Placeholder)")) {
                // TODO
                // for (long long id : selected_ids) { entityManager.rotate(id, angle); }
            }

            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();
    // END
}

void ObjectListener::DrawTransformCombination() {
    // TRANSFORM LIST
    static int selected = 0;
    //std::vector<char*> transform_buf(100, "Transformation");
    {
        ImGui::BeginChild("transform list", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
        ImGui::Text("Transorms list"); ImGui::Separator();
        for (int i = 0; i < transform_buf_names.size(); i++)
        {
            char label[128];
            sprintf(label, transform_buf_names[i]);
            ImGui::PushID(i);
            if (ImGui::Selectable(label, selected == i, ImGuiSelectableFlags_SelectOnNav))
                selected = i;
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();

    // NEW TRANSFORM INPUTS
    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
    ImGui::Text("Add new transformation"); ImGui::Separator();

    static float fsx, fsy;
    ImGui::Text("Scaling");
    ImGui::Text("x: "); ImGui::SameLine();
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("##sx", &fsx, 1.0f, 0.0f, 0.0f, "%.06f"); 
    ImGui::PopItemWidth(); ImGui::SameLine();
    ImGui::Text("y: "); ImGui::SameLine();
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("##sy", &fsy, 1.0f, 0.0f, 0.0f, "%.06f");
    ImGui::PopItemWidth(); ImGui::SameLine();
    ImGui::Text("  "); ImGui::SameLine();
    ImGui::PushID("add_scaling");
    if (ImGui::Button("Add", DFM_BUTTON_SIZE)) {
        HandleAddScaling(fsx, fsy);//Handle scaling addition;
    }
    ImGui::PopID();
    ImGui::Separator();

    static float ftx, fty;
    ImGui::Text("Translation");
    ImGui::Text("x: "); ImGui::SameLine();
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("##tx", &ftx, 1.0f, 0.0f, 0.0f, "%.06f"); 
    ImGui::PopItemWidth(); ImGui::SameLine();
    ImGui::Text("y: "); ImGui::SameLine();
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("##ty", &fty, 1.0f, 0.0f, 0.0f, "%.06f");
    ImGui::PopItemWidth(); ImGui::SameLine();
    ImGui::Text("  "); ImGui::SameLine();
    ImGui::PushID("add_translation");
    if (ImGui::Button("Add", DFM_BUTTON_SIZE)) {
        HandleAddTranslation(ftx, fty);//Handle transform addition;
    }
    ImGui::PopID();
    ImGui::Separator();
    
    static float fangle, frx, fry;
    ImGui::Text("Rotation");
    //ImGui::Text("Around: "); ImGui::SameLine();
    static int radiosel;
    if (ImGui::RadioButton("itself", &radiosel, 0)) {
            ; // passa as coordenadas do centro do objeto para frx e fry
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("origin", &radiosel, 1)) {
           frx = 0.0f; fry = 0.0f;// passa coordenadas da origem para frx e fry
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("an arbitrary point", &radiosel, 2)) {
            ; // da unlock em frx e fry
        }

    ImGui::Text("x: "); ImGui::SameLine();
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("##rx", &frx, 1.0f, 0.0f, 0.0f, "%.06f"); 
    ImGui::PopItemWidth(); ImGui::SameLine();
    ImGui::Text("y: "); ImGui::SameLine();
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("##ry", &fry, 1.0f, 0.0f, 0.0f, "%.06f");
    ImGui::PopItemWidth(); //ImGui::SameLine();
    ImGui::Text("angle: "); ImGui::SameLine();
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("##ra", &fangle, 1.0f, 0.0f, 360.0f, "%.06f", ImGuiSliderFlags_WrapAround); 
    ImGui::PopItemWidth(); ImGui::SameLine();
    ImGui::Text("  "); ImGui::SameLine();
    ImGui::PushID("add_rotation");
    if (ImGui::Button("Add", DFM_BUTTON_SIZE)) {
        HandleAddRotation(frx, fry, fangle);
    }
    ImGui::PopID();
    ImGui::Separator();

    ImGui::Button("Apply all transformations");

    ImGui::EndChild();
}

inline std::string get_selected_ids(const std::unordered_set<long long> &ids){
    std::string selected_objects;
    if(ids.size() > 1){
        selected_objects = "Selected objects IDs: ";
        selected_objects += '(';
        bool sep = false;
        for(const auto &i: ids){
            if(sep) selected_objects += ", ";
            sep = true;
            selected_objects += std::to_string(i/10); // divide por 10 para mostrar o fake_id
        }
        selected_objects += ')';
    } else selected_objects = "Selected object ID: "+std::to_string(*ids.begin()/10);
    return selected_objects;
}

std::vector<std::string> split_string(const std::string &s, char split_char){
    std::vector<std::string> res;
    std::string current_string = "";
    for(char e: s){
        if(e == split_char){
            res.push_back(current_string);
            current_string = "";
        } else current_string.push_back(e);
    }
    res.push_back(current_string);
    return res;
}

void ObjectListener::DrawWindow() {
    ImGui::SetNextWindowPos(ImVec2(877, 267), ImGuiCond_FirstUseEver); 
    ImGui::SetNextWindowSize(ImVec2(786, 336), ImGuiCond_FirstUseEver); // switch to percentage
    ImGui::Begin("Display File Manifest");
    DrawObjectList(); ImGui::SameLine();

    // DEMO TEMPLATE
    {
        ImGui::BeginGroup();
        ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
        if (!selected_ids.empty()){
            ImGui::Text("%s", get_selected_ids(selected_ids).c_str()); 
        } else
            ImGui::Text("No object is selected");
        ImGui::Separator();
        if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Details")) {
                if (ImGui::BeginTable("DetailsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) { 
                    ImGui::TableSetupColumn("Type");
                    ImGui::TableSetupColumn("ID");
                    ImGui::TableSetupColumn("Points");
                    ImGui::TableHeadersRow();

                    for (const auto& id : selected_ids) {
                        // Retrieve the object details from the EntityManager
                        std::string object_details = entityManager.GetObjectDetails(id);
                        // Split the string into columns
                        auto columns = split_string(object_details, DEFAULT_SPLIT_CHAR);

                        // Populate the table row
                        ImGui::TableNextRow();
                        for (size_t i = 0; i < columns.size(); ++i) {
                            ImGui::TableSetColumnIndex(i);
                            ImGui::Text("%s", columns[i].c_str());
                        }
                    }

                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Transform Combination")) {
                DrawTransformCombination();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();
        ImGui::EndGroup();
    }

    ImGui::End();
}