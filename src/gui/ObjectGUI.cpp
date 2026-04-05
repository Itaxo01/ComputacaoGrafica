#include "ObjectGUI.hpp"
#include <string>
#include <algorithm>
//#include "Util.hpp"

#define DFM_INPUT_BOX_SIZE 100
#define DFM_BUTTON_SIZE ImVec2(50.0f, 20.0f)
#define DEFAULT_SPLIT_CHAR '|'

const char* ObjectGUI::GetTypeName(core::ShapeType type) {
    switch (type) {
        case core::ShapeType::POINT: return "Point";
        case core::ShapeType::LINE: return "Line";
        case core::ShapeType::WIREFRAME: return "Wireframe";
        default: return "Unknown";
    }
}

void ObjectGUI::DrawObjectList() {
    std::vector<ManifestEntry> manifest = entityManager.GetManifest();
    std::vector<std::string> names = entityManager.GetObjectNames();
    std::vector<long long> ids = entityManager.GetObjectIDs();

    ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);  
    
    // Monta label para display na lista usando o nome, tipo e fake_id do objeto
    std::vector<std::string> labels;
    for (auto& entry : manifest) {
        std::string label = "[" + std::to_string(entry.fake_id) + "] " + entry.name + " (" + GetTypeName(entry.type) + ")";
        labels.push_back(label);
    }

    // Inicializa a lista de seleção múltipla com os nomes dos objetos e operações de right click
    std::vector<std::string> context_item_names = {"Delete", "Rotate (Placeholder)"};
    multipleSelectionList.SetNames(labels);
    multipleSelectionList.SetContextItems(context_item_names);
    multipleSelectionList.Draw();

    // Captura IDs do itens selecionados
    std::unordered_set<int> selected_indexes = multipleSelectionList.GetSelectedIndexes();
    for (int i : selected_indexes) {
        selected_ids.insert(ids[i]);
    }

    // Captura operação selecionada com o botão direito e direciona tratamento para o controller
    int selected_context_item = multipleSelectionList.GetSelectedContextItem();
    switch (selected_context_item) {
        case 0: // Delete
            for (const auto& id : selected_ids) {
                entityManager.remove(id);
            }
            selected_ids.clear();
            multipleSelectionList.clear();
            break;
        case 1: // Rotate (Placeholder)
            // Implement rotation logic here, possibly by opening another window or applying a default rotation
            break;
        default:
            break;
    }

    ImGui::EndChild();
}

inline void ObjectGUI::DrawAddScaling() {
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
        objectController.HandleAddScaling(fsx, fsy);//Handle scaling addition;
    }
    ImGui::PopID();
}

inline void ObjectGUI::DrawAddTranslation() {
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
        objectController.HandleAddTranslation(ftx, fty);//Handle transform addition;
    }
    ImGui::PopID();
}

inline void ObjectGUI::DrawAddRotation() {
    static float fangle, frx, fry;
    static int radiosel;

    ImGui::Text("Rotation");
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
        objectController.HandleAddRotation(frx, fry, fangle);
    }
    ImGui::PopID();
}

void ObjectGUI::DrawTransformCombination() {
    // TRANSFORM LIST
    //static int selected = 0;
    //std::vector<char*> transform_buf(100, "Transformation");
    {
        ImGui::BeginChild("transform list", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
        ImGui::Text("Transorms list"); ImGui::Separator();
        /*for (int i = 0; i < transform_buf_names.size(); i++)
        {
            char label[128];
            sprintf(label, transform_buf_names[i]);
            ImGui::PushID(i);
            if (ImGui::Selectable(label, selected == i, ImGuiSelectableFlags_SelectOnNav))
                selected = i;
            ImGui::PopID();
        }*/
        ImGui::EndChild();
    }
    ImGui::SameLine();

    // Desenha todos os inputs para adição de transformações
    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
    ImGui::Text("Add new transformation"); ImGui::Separator();

    DrawAddScaling();
    ImGui::Separator();
    DrawAddTranslation();
    ImGui::Separator();
    DrawAddRotation();
    ImGui::Separator();

    if (ImGui::Button("Apply all transformations")) {
        objectController.ApplyTransformations();
    }

    ImGui::EndChild();
}

inline std::string get_selected_idsTEMP(const std::unordered_set<long long> &ids){
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

std::vector<std::string> siplit_stringTEMP(const std::string &s, char split_char){
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

void ObjectGUI::DrawObjectDetails() {
    if (ImGui::BeginTable("DetailsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) { 
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Points");
        ImGui::TableHeadersRow();

        for (const auto& id : selected_ids) {
            // Retrieve the object details from the EntityManager
            std::string object_details = entityManager.GetObjectDetails(id);
            // Split the string into columns
            auto columns = siplit_stringTEMP(object_details, DEFAULT_SPLIT_CHAR);

            // Populate the table row
            ImGui::TableNextRow();
            for (size_t i = 0; i < columns.size(); ++i) {
                ImGui::TableSetColumnIndex(i);
                ImGui::Text("%s", columns[i].c_str());
            }
        }
        ImGui::EndTable();
    }
}

void ObjectGUI::DrawWindow() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    ImGui::SetNextWindowPos(ImVec2(work_pos.x + work_size.x * (877.0f / 1920.0f), work_pos.y + work_size.y * (267.0f / 1080.0f)), ImGuiCond_FirstUseEver); 
    ImGui::SetNextWindowSize(ImVec2(work_size.x * (786.0f / 1920.0f), work_size.y * (336.0f / 1080.0f)), ImGuiCond_FirstUseEver); // switch to percentage
    ImGui::Begin("Display File Manifest");
    DrawObjectList(); ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us

    // Display selected object IDs
    if (!selected_ids.empty()){
        ImGui::Text("%s", get_selected_idsTEMP(selected_ids).c_str()); 
    } else
        ImGui::Text("No object is selected");
    ImGui::Separator();

    // Tabs for Details and Transform Combination
    if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Details")) {
            DrawObjectDetails();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Transform Combination")) {
            ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
            DrawTransformCombination();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::EndChild();
    ImGui::EndGroup();
    ImGui::End();
}

