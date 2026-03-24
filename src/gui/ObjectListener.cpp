#include "ObjectListener.hpp"
#include <string>
#include <algorithm>

const char* ObjectListener::GetTypeName(core::ShapeType type) {
    switch (type) {
        case core::ShapeType::POINT: return "Point";
        case core::ShapeType::LINE: return "Line";
        case core::ShapeType::WIREFRAME: return "Wireframe";
        default: return "Unknown";
    }
}

void ObjectListener::DrawWindow() {
    ImGui::SetNextWindowPos(ImVec2(877, 257), ImGuiCond_FirstUseEver); 
    ImGui::SetNextWindowSize(ImVec2(365, 330), ImGuiCond_FirstUseEver); // switch to percentage
    ImGui::Begin("Display File Manifest");
        const auto& manifest = entityManager.GetManifest();

        for (int i = 0; i < manifest.size(); ++i) {
            const auto& entry = manifest[i];
            
            // Check if current item is in our selected set
            bool is_selected = selected_ids.find(entry.id) != selected_ids.end();
            
            // Format the display label
            std::string label = "[" + std::to_string(entry.id) + "] " + entry.name + " (" + GetTypeName(entry.type) + ")";

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
    ImGui::End();
}