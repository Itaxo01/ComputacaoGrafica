#include "MultipleSelectionList.hpp"

void MultipleSelectionList::Draw() {
    if (!names_ptr) return; // Early return if no names set
    for (int index = 0; index < names_ptr->size(); ++index) {

        // Check if current item is in our selected set
        bool is_selected = selected_indexes.find(index) != selected_indexes.end();

        // Draw the selectable item
        if (ImGui::Selectable((*names_ptr)[index], is_selected)) {
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
            for (int j = 0; j < context_item_names.size(); ++j) {
                if (ImGui::MenuItem(context_item_names[j])) {
                    selected_context_item = j;
                    // Context Item Handle now need to be captured by the user of this class 
                    // by checking the value of selected_context_item after calling Draw()
                }
            }
            ImGui::EndPopup();
        }
    }
}

const std::unordered_set<int>& MultipleSelectionList::GetSelectedIndexes() {
    return selected_indexes;
}

void MultipleSelectionList::AddContextItem(const char* name) {
    context_item_names.push_back(name);
}

