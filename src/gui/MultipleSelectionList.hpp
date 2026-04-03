#ifndef MULTIPLE_SELECTION_LIST_HPP
#define MULTIPLE_SELECTION_LIST_HPP

#include "imgui.h"
#include <unordered_set>
#include <vector>
#include <algorithm> // for min and max
#include <string>

class MultipleSelectionList {
private:
    int size = 0;
    std::unordered_set<int> selected_indexes;
    std::vector<const char*>* names_ptr;
    std::vector<const char*> context_item_names;
    int last_selected_index = -1;
    int selected_context_item = -1;
public:
    MultipleSelectionList() : size(0), selected_indexes(), names_ptr(nullptr), context_item_names() {}
    void SetNames(std::vector<const char*>& names) { names_ptr = &names; }
    void Draw();
    const std::unordered_set<int>& GetSelectedIndexes();
    void AddContextItem(const char* name);
    int GetSelectedContextItem(const std::vector<const char*>& context_items) {
        return selected_context_item;
    }
    void SetContextItems(const std::vector<const char*>& items) {
        context_item_names = items;
    }
    void clear() {
        selected_indexes.clear();
        last_selected_index = -1;    
    }
};

#endif