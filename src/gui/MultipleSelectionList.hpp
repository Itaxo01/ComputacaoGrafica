#ifndef MULTIPLE_SELECTION_LIST_HPP
#define MULTIPLE_SELECTION_LIST_HPP

#include "imgui.h"
#include <functional>
#include <unordered_set>
#include <vector>
#include <algorithm> // for min and max
#include <string>

class MultipleSelectionList {
private:
    int size = 0;
    std::unordered_set<int> selected_indexes;
    std::vector<std::string> context_item_names;
    int last_selected_index = -1;
    int selected_context_item = -1;

    unsigned int current_page = 0;
    const int items_per_page = 20;

    std::function<std::string(int)> get_name; // Função recebida em SetData

public:
    MultipleSelectionList() : size(0), selected_indexes(), context_item_names() {}
    void SetData(int size, std::function<std::string(int)> callback) {
        this->size = size;
        this->get_name = callback;
    }
    
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