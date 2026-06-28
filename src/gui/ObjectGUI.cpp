#include "ObjectGUI.hpp"
#include "Object.hpp"
#include "AppConfig.hpp"
#include "GuiLayout.hpp"
#include "ObjectMetadatas/CurveMetadata.hpp"
#include "ObjectMetadatas/SurfaceMetadata.hpp"
#include <string>
#include <algorithm>
#include <cstdio>
//#include "Util.hpp"

#define DFM_INPUT_BOX_SIZE 100
#define DFM_VEC_WIDTH 200
#define DFM_LABEL_COL 70.0f
#define DFM_BUTTON_SIZE ImVec2(50.0f, 20.0f)

// DragFloat3 no modo 3D, DragFloat2 no 2D (mantém o componente z inalterado no 2D).
static bool DragVec(const char* id, float v[3], float speed) {
    ImGui::PushItemWidth(DFM_VEC_WIDTH);
    bool changed = AppConfig::is3d
        ? ImGui::DragFloat3(id, v, speed, 0.0f, 0.0f, "%.2f")
        : ImGui::DragFloat2(id, v, speed, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    return changed;
}

const char* ObjectGUI::GetTypeName(core::ObjectType type) {
    return core::getTypeName(type);
}

void ObjectGUI::DrawObjectList() {
    const auto& objects = entityManager.getObjects();

    multipleSelectionList.SetData(objects.size(), [&](int index){
        const auto& obj = objects[index];
        std::string label = "[" + std::to_string(obj.id) + "] " + obj.name + " (" + GetTypeName(obj.type) + ")";
        return label;
    });

    ImGui::BeginChild("left pane", ImVec2(280 * gui::layout::Scale(), 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);

    std::vector<std::string> context_item_names = {"Edit", "Delete", "Rotate (Placeholder)"};
    std::vector<bool>        context_single_only = {true,   false,    false};
    multipleSelectionList.SetContextItems(context_item_names, context_single_only);

    // Select All / Delete All sit on the pagination row, right-aligned as a group
    // (both disabled when there are no objects).
    bool has_objects = !objects.empty();
    multipleSelectionList.SetHeaderAction([this, has_objects]() {
        const char* sel_label = "Select All";
        const char* del_label = "Delete All";
        ImGuiStyle& style = ImGui::GetStyle();
        float sel_w = ImGui::CalcTextSize(sel_label).x + style.FramePadding.x * 2.0f;
        float del_w = ImGui::CalcTextSize(del_label).x + style.FramePadding.x * 2.0f;
        float group_w = sel_w + style.ItemSpacing.x + del_w;
        float avail = ImGui::GetContentRegionAvail().x;
        if (avail > group_w) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - group_w));

        if (!has_objects) ImGui::BeginDisabled();
        if (ImGui::Button(sel_label)) {
            multipleSelectionList.selectAll();
        }
        ImGui::SameLine();
        if (ImGui::Button(del_label)) {
            size_t n = entityManager.getObjects().size();
            entityManager.removeAll();
            selected_ids.clear();
            multipleSelectionList.clear();
            log.AddLog("Deleted all objects (%zu)\n", n);
        }
        if (!has_objects) ImGui::EndDisabled();
    });

    multipleSelectionList.Draw();

    selected_ids.clear();
    for (int i : multipleSelectionList.GetSelectedIndexes()) {
        selected_ids.insert(objects[i].id);
    }
    objectController.SetSelectedIDs(selected_ids);

    // Captura operação selecionada com o botão direito e direciona tratamento para o controller
    int selected_context_item = multipleSelectionList.GetSelectedContextItem();
    switch (selected_context_item) {
        case 0: // Edit — single selection only (enforced as disabled in the menu)
            if (selected_ids.size() == 1) {
                long long edit_id = *selected_ids.begin();
                core::Object& obj = entityManager.getObject(edit_id);
                int  edit_method = 0;
                int  edit_cols   = 0;
                bool edit_filled = obj.material.filled;
                if (obj.type == core::ObjectType::CURVE2D) {
                    const CurveMetadata* meta = entityManager.getCurveMetadata(edit_id);
                    if (meta) edit_method = meta->method;
                } else if (obj.type == core::ObjectType::SURFACE) {
                    const SurfaceMetadata* meta = entityManager.getSurfaceMetadata(edit_id);
                    if (meta) { edit_method = meta->method; edit_cols = meta->cols; }
                }
                auto pts = entityManager.GetObjectRawPoints(edit_id);
                editing_id = edit_id;
                point_editor.OpenForEdit(obj.type, edit_method, edit_filled, pts, edit_cols);
            }
            break;
        case 1: // Delete
            for (const auto& id : selected_ids) {
                entityManager.remove(id);
            }
            selected_ids.clear(); // Ver se é realmente necessário, já que o clear acontecerá denovo na próxima captura de IDs.
            multipleSelectionList.clear();
            break;
        case 2: // Rotate (Placeholder)
            // Implement rotation logic here, possibly by opening another window or applying a default rotation
            break;
        default:
            break;
    }

    ImGui::EndChild();
}

// Linha compacta "Label  [vec3/vec2]  [Add]". Mantém as três transformações com
// a mesma cara e mostra o componente z apenas no modo 3D.
inline void ObjectGUI::DrawAddScaling() {
    static float s[3] = {1.0f, 1.0f, 1.0f};
    ImGui::TextUnformatted("Scale"); ImGui::SameLine(DFM_LABEL_COL);
    DragVec("##scl", s, 0.05f); ImGui::SameLine();
    ImGui::PushID("add_scaling");
    if (ImGui::Button("Add", DFM_BUTTON_SIZE)) {
        objectController.HandleAddScaling(s[0], s[1], AppConfig::is3d ? s[2] : 1.0f);
    }
    ImGui::PopID();
}

inline void ObjectGUI::DrawAddTranslation() {
    static float t[3] = {0.0f, 0.0f, 0.0f};
    ImGui::TextUnformatted("Translate"); ImGui::SameLine(DFM_LABEL_COL);
    DragVec("##trn", t, 1.0f); ImGui::SameLine();
    ImGui::PushID("add_translation");
    if (ImGui::Button("Add", DFM_BUTTON_SIZE)) {
        objectController.HandleAddTranslation(t[0], t[1], AppConfig::is3d ? t[2] : 0.0f);
    }
    ImGui::PopID();
}

inline void ObjectGUI::DrawAddRotation() {
    static float angle = 0.0f;
    static float center[3] = {0.0f, 0.0f, 0.0f};
    static int   centerMode = 0; // 0 = itself, 1 = origin, 2 = arbitrary point
    static int   axisSel = 2;    // 0 = X, 1 = Y, 2 = Z (apenas Z no modo 2D)

    // Linha 1: ângulo (+ eixo no 3D) + botão Add.
    ImGui::TextUnformatted("Rotate"); ImGui::SameLine(DFM_LABEL_COL);
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("##angle", &angle, 1.0f, 0.0f, 360.0f, "%.1f deg", ImGuiSliderFlags_WrapAround);
    ImGui::PopItemWidth(); ImGui::SameLine();
    if (AppConfig::is3d) {
        ImGui::PushItemWidth(50);
        ImGui::Combo("##axis", &axisSel, "X\0Y\0Z\0");
        ImGui::PopItemWidth(); ImGui::SameLine();
    } else {
        axisSel = 2; // 2D sempre roda em torno de Z
    }
    ImGui::PushID("add_rotation");
    if (ImGui::Button("Add", DFM_BUTTON_SIZE)) {
        float axis[3][3] = {{1,0,0}, {0,1,0}, {0,0,1}};
        objectController.HandleAddRotation(center[0], center[1], center[2],
                                           axis[axisSel][0], axis[axisSel][1], axis[axisSel][2],
                                           angle);
    }
    ImGui::PopID();

    // Linha 2: centro de rotação (itself / origin / arbitrary).
    ImGui::TextUnformatted("around"); ImGui::SameLine(DFM_LABEL_COL);
    ImGui::RadioButton("itself", &centerMode, 0); ImGui::SameLine();
    ImGui::RadioButton("origin", &centerMode, 1); ImGui::SameLine();
    ImGui::RadioButton("point",  &centerMode, 2);

    if (centerMode == 0) {
        auto [cx, cy, cz] = objectController.GetSelectedObjectsCenter();
        center[0] = cx; center[1] = cy; center[2] = cz;
    } else if (centerMode == 1) {
        center[0] = center[1] = center[2] = 0.0f;
    } else { // arbitrary point: editable on its own row
        ImGui::TextUnformatted("point"); ImGui::SameLine(DFM_LABEL_COL);
        DragVec("##center", center, 1.0f);
    }
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
    
    ImGui::BeginChild("transform list", ImVec2(150 * gui::layout::Scale(), 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
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
    ImGui::BeginChild("item view", ImVec2(400 * gui::layout::Scale(), -ImGui::GetFrameHeightWithSpacing()));
    ImGui::Text("Add new transformation"); ImGui::Separator();

    DrawAddScaling();
    ImGui::Separator();
    DrawAddTranslation();
    ImGui::Separator();
    DrawAddRotation();
    ImGui::Separator();

    // Duração da transição: 0 = aplica instantaneamente; > 0 anima a 60 fps.
    static float duration = 0.0f;
    ImGui::PushItemWidth(DFM_INPUT_BOX_SIZE);
    ImGui::DragFloat("Duration (s)", &duration, 0.05f, 0.0f, 60.0f, "%.2f");
    ImGui::PopItemWidth();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("0 = instant; > 0 plays the transition smoothly over this many seconds");
    ImGui::SameLine();
    if (ImGui::Button("Apply all transformations")) {
        objectController.ApplyTransformations(duration);
    }

    DrawTransformImport();

    ImGui::EndChild();
}

// Importação de transformações por texto ou ficheiro. A lógica vive no controller
// (ApplyTransformationScript); aqui é só a view. O preset "Donut spin" preenche o
// script da rosquinha giratória famosa.
void ObjectGUI::DrawTransformImport() {
    static const char* kDonutScript =
        "# Famous rotating donut: tumbles forever around two axes.\n"
        "duration 6\n"
        "loop\n"
        "rotate 360 axis 1 0 0 around itself\n"
        "rotate 360 axis 0 0 1 around itself\n";

    static char   script[1024] = "";
    static char   path[512]    = "models/donut_spin.txt";
    static std::string status;

    ImGui::Separator();
    if (!ImGui::CollapsingHeader("Import transformations")) return;

    // Importar de um ficheiro.
    ImGui::PushItemWidth(DFM_VEC_WIDTH);
    ImGui::InputText("##scriptpath", path, sizeof(path));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Load file")) {
        status = objectController.ApplyTransformationScriptFile(path);
        if (status.empty()) status = "Applied script from file.";
    }

    // Importar de texto.
    ImGui::InputTextMultiline("##scripttext", script, sizeof(script),
                              ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 6));
    if (ImGui::Button("Apply script")) {
        status = objectController.ApplyTransformationScript(script);
        if (status.empty()) status = "Applied script from text.";
    }
    ImGui::SameLine();
    if (ImGui::Button("Donut spin")) {
        snprintf(script, sizeof(script), "%s", kDonutScript);
        status = objectController.ApplyTransformationScript(script);
        if (status.empty()) status = "Donut spinning! (3D mode, object selected)";
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop animation")) {
        objectController.StopAnimations();
        status = "Stopped.";
    }

    if (!status.empty()) {
        ImGui::TextWrapped("%s", status.c_str());
    }
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
            selected_objects += std::to_string(i);
            count++;
            if(count > 20){
                selected_objects += "...";
                break;
            }
        }
        selected_objects += ')';
    } else selected_objects = "Selected object ID: "+std::to_string(*ids.begin());
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
    auto r = gui::layout::Get(gui::layout::Region::ObjectManager);
    ImGui::SetNextWindowPos(r.pos, gui::layout::Cond());
    ImGui::SetNextWindowSize(r.size, gui::layout::Cond());
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
            ImGui::BeginChild("left pane", ImVec2(550 * gui::layout::Scale(), 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
            DrawTransformCombination();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::EndChild();
    ImGui::EndGroup();
    ImGui::End();

    // Edit-points modal — must be called every frame (outside the window block)
    {
        std::vector<std::tuple<float,float,float>> new_pts;
        core::ObjectType new_type   = core::ObjectType::POINT;
        int             new_method = 0;
        bool            new_filled = false;
        int             new_rows = 0, new_cols = 0;
        if (point_editor.DrawModal(new_pts, new_type, new_method, new_filled, new_rows, new_cols) && editing_id != -1) {
            entityManager.UpdateObjectPoints(editing_id, new_pts, new_type, new_method, new_filled, new_rows, new_cols);
            editing_id = -1;
            selected_ids.clear();
            multipleSelectionList.clear();
        }
    }
}

