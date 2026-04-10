#include "MultipleSelectionList.hpp"
#include "imgui.h"

void MultipleSelectionList::Draw() {

    // REFAZER ESQUEMA DE PAGINACAO
    int total_items = size;
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
    
    for (int index = start_idx; index < end_idx; ++index) {

        // Check if current item is in our selected set
        bool is_selected = selected_indexes.find(index) != selected_indexes.end();

        // Draw the selectable item
        if (ImGui::Selectable(get_name(index).c_str(), is_selected)) {
            ImGuiIO& io = ImGui::GetIO();
            
            if (io.KeyCtrl) {
                // CTRL+Click: Toggle selection
                if (is_selected) selected_indexes.erase(index);
                else selected_indexes.insert(index);
                last_selected_index = index;
            } 
            else if (io.KeyShift && last_selected_index != -1) {
                // SHIFT+Click: Select range
                selected_indexes.clear();
                int start = std::min(last_selected_index, index);
                int end = std::max(last_selected_index, index);
                for (int j = start; j <= end; ++j) {
                    selected_indexes.insert(j);
                }
            } 
            else {
                // Normal Click: Clear others, select this one
                selected_indexes.clear();
                selected_indexes.insert(index);
                last_selected_index = index;
            }
        }

        // --- CONTEXT MENU (Right Click) ---
        // We tie the context menu to the item. It opens if you right click a hovered item.
        if (ImGui::BeginPopupContextItem(("context_menu_" + std::to_string(index)).c_str())){
            
            // If the user right-clicks an unselected item, select ONLY that item
            if (!is_selected) {
                selected_indexes.clear();
                selected_indexes.insert(index);
                last_selected_index = index;
            }

            ImGui::Text("Operations (%zu selected)", selected_indexes.size());
            ImGui::Separator();

            selected_context_item = -1; // Mudar de lugar depois?
            for (int j = 0; j < (int)context_item_names.size(); ++j) {
                if (ImGui::MenuItem(context_item_names[j].c_str())) {
                    selected_context_item = j;
                    // Context Item Handle now need to be captured by the user of this class 
                    // by checking the value of selected_context_item after calling Draw()
                }
            }
            ImGui::EndPopup();
        }
    }
    ImGui::EndChild();
}

std::unordered_set<int> MultipleSelectionList::GetSelectedIndexes() {
    return selected_indexes;
}

/*void MultipleSelectionList::AddContextItem(const char* name) {
    context_item_names.push_back(name);
}*/