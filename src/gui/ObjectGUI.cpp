#include "ObjectGUI.hpp"
#include <string>
#include <algorithm>
//#include "Util.hpp"

#define DFM_INPUT_BOX_SIZE 100
#define DFM_BUTTON_SIZE ImVec2(50.0f, 20.0f)

const char* ObjectGUI::GetTypeName(core::ShapeType type) {
    switch (type) {
        case core::ShapeType::POINT:        return "Point";
        case core::ShapeType::LINE:         return "Line";
        case core::ShapeType::WIREFRAME:    return "Wireframe";
        case core::ShapeType::POLYGON:      return "Polygon";
        case core::ShapeType::BEZIER_CURVE: return "Bezier Curve";
        default:                            return "Unknown";
    }
}

void ObjectGUI::DrawObjectList() {
    const auto &manifest = entityManager.GetManifest();
    // Em vez de pegar todas as strings, mandar em tempo real as utilizadas pelo MultipleSelectionList usando uma lambda function. Dessa forma, não é necessário passar a lista inteira de nomes para o objeto.

    multipleSelectionList.SetData(manifest.size(), [&](int index){
        const auto& entry = manifest[index];
        std::string label = "[" + std::to_string(entry.fake_id) + "] " + entry.name + " (" + GetTypeName(entry.type) + ")";
        return label;
    });
    // Monta label para display na lista usando o nome, tipo e fake_id do objeto
    

    ImGui::BeginChild("left pane", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);  
    
    // Inicializa a lista de seleção múltipla com os nomes dos objetos e operações de right click
    std::vector<std::string> context_item_names = {"Delete", "Rotate (Placeholder)"};
    multipleSelectionList.SetContextItems(context_item_names);
    multipleSelectionList.Draw();

    // Captura IDs do itens selecionados
    selected_ids.clear();
    for (int i :  multipleSelectionList.GetSelectedIndexes()) {
        selected_ids.insert(manifest[i].id);
    }
    objectController.SetSelectedIDs(selected_ids); // Redundante?

    // Captura operação selecionada com o botão direito e direciona tratamento para o controller
    int selected_context_item = multipleSelectionList.GetSelectedContextItem();
    switch (selected_context_item) {
        case 0: // Delete
            for (const auto& id : selected_ids) {
                entityManager.remove(id);
            }
            selected_ids.clear(); // Ver se é realmente necessário, já que o clear acontecerá denovo na próxima captura de IDs.
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
            ;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("origin", &radiosel, 1)) {
            ;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("an arbitrary point", &radiosel, 2)) {
            ;
        }
    
        switch(radiosel) {
            case 0: { // ITSELF
                auto [center_x, center_y, center_z] = objectController.GetSelectedObjectsCenter();
                frx = center_x; fry = center_y;
                break;
            }
            case 1: { // ORIGIN
                frx = 0.0f; fry = 0.0f;
                break;
            }
            case 2: // ARBITRARY POINT
                break;
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

inline void DrawMatrix(core::mat4 &matrix) {
    if (ImGui::BeginTable("DetailsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) { 
        ImGui::TableSetupColumn("0");
        ImGui::TableSetupColumn("1");
        ImGui::TableSetupColumn("2");
        ImGui::TableHeadersRow();

        for (int i = 0; i < 3; ++i) {
            // Populate the table row
            ImGui::TableNextRow();
            for (size_t j = 0; j < 3; ++j) {
                ImGui::TableSetColumnIndex(j);
                ImGui::Text("%.2f", matrix[i][j]);
            }
        }
        ImGui::EndTable();
    }
}

void ObjectGUI::DrawTransformCombination() {
    
    ImGui::BeginChild("transform list", ImVec2(150, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
    ImGui::Text("transforms list"); ImGui::Separator();

    std::vector<char*> transform_buf_names = objectController.GetTransformationBufferNames();
    // Atualmente convertendo char* para string. Talvez seja bom no futuro redefinir names para char* no MultipleSelectionList
    transformationsList.SetData(transform_buf_names.size(), [&](int index) {
        return std::string(transform_buf_names[index]);
    });

    // transformationsList.SetContextItems(context_item_names);
    transformationsList.Draw();

    std::unordered_set<int> selected_transformations = transformationsList.GetSelectedIndexes();
    objectController.SetSelectedTransfomations(selected_transformations);

    if (view_matrix_popup_open) {
        DrawMatrix(matrix_to_view);
    }

    int selected_context_item = transformationsList.GetSelectedContextItem();
    switch (selected_context_item) {
        case 0: // Delete
            // Implement deletion of selected transformations from the buffer
            {

            }
            break;
        case 1: // View Matrix
            // Implement logic to display the matrix of the selected transformation, possibly in a popup or a new window
            {
                matrix_to_view = objectController.GetSelectedTransformationMatrix();
                view_matrix_popup_open = true; // Set flag to open popup
            }
            break;
        default:
            break;
    }

    ImGui::EndChild();
    ImGui::SameLine();

    // Desenha todos os inputs para adição de transformações
    ImGui::BeginChild("item view", ImVec2(400, -ImGui::GetFrameHeightWithSpacing()));
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
        int count = 0;
        for(const auto &i: ids){
            if(sep) selected_objects += ", ";
            sep = true;
            selected_objects += std::to_string(i/10); // divide por 10 para mostrar o fake_id
            count++;
            if(count > 20){
                selected_objects += "...";
                break;
            }
        }
        selected_objects += ')';
    } else selected_objects = "Selected object ID: "+std::to_string(*ids.begin()/10);
    return selected_objects;
}

void ObjectGUI::DrawObjectDetails() {
    if (ImGui::BeginTable("DetailsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) { 
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Color");
        ImGui::TableSetupColumn("Points");
        ImGui::TableHeadersRow();

        for (const auto& id : selected_ids) {
            // Retrieve the object details from the EntityManager
            core::ObjectDetails object_details = entityManager.GetObjectDetails(id);

            // Populate the table row
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", object_details.type.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", object_details.id.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%s", object_details.name.c_str());
            ImGui::TableSetColumnIndex(3); ImGui::Text("%s", object_details.color.c_str());
            ImGui::TableSetColumnIndex(4); ImGui::Text("%s", object_details.points.c_str());
        }
        ImGui::EndTable();
    }
}

void ObjectGUI::DrawWindow() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 monitor_pos = viewport->Pos;
    ImVec2 monitor_size = viewport->Size;

    // Proportional window configurations based on the app window/monitor size
    ImGui::SetNextWindowPos(ImVec2(monitor_pos.x + monitor_size.x * (899.0f / 1700.0f), monitor_pos.y + monitor_size.y * (231.0f / 940.0f)), ImGuiCond_FirstUseEver); 
    ImGui::SetNextWindowSize(ImVec2(monitor_size.x * (731.0f / 1700.0f), monitor_size.y * (374.0f / 940.0f)), ImGuiCond_FirstUseEver); 
    ImGui::Begin("Object Manager");
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
            ImGui::BeginChild("left pane", ImVec2(550, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
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

