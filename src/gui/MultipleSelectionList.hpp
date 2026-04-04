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
    std::vector<std::string> names;
    std::vector<std::string> context_item_names;
    int last_selected_index = -1;
    int selected_context_item = -1;
public:
    MultipleSelectionList() : size(0), selected_indexes(), names(), context_item_names() {}
    void SetNames(std::vector<std::string>& names) { this->names = names; }
    void Draw();
    std::unordered_set<int> GetSelectedIndexes();
    //void AddContextItem(std::string name);
    int GetSelectedContextItem() {
        int temp = selected_context_item;
        selected_context_item = -1; // Reset after reading
        return temp;
    }
    void SetContextItems(const std::vector<std::string>& items) {
        context_item_names = items;
    }
    void clear() {
        selected_indexes.clear();
        last_selected_index = -1;    
    }
};

#endif