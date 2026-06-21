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
    std::vector<bool> context_item_single_only; // item enabled only when exactly 1 selected
    int last_selected_index = -1;
    int selected_context_item = -1;

    int current_page = 0;
    const int items_per_page = 20;

    int just_clicked = -1; // index of the last item plain-clicked (no modifiers), reset after read

    std::function<std::string(int)> get_name; // Função recebida em SetData
    std::function<void()> header_action;      // widget opcional desenhado na linha da paginação

public:
    MultipleSelectionList() : size(0), selected_indexes(), context_item_names() {}
    void SetData(int size, std::function<std::string(int)> callback) {
        this->size = size;
        this->get_name = callback;
    }
    // Optional widget drawn on the pagination row, after the page controls (e.g. a
    // right-aligned action button). Left unset for lists that don't need one.
    void SetHeaderAction(std::function<void()> fn) {
        this->header_action = std::move(fn);
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
        context_item_single_only.assign(items.size(), false);
    }
    // single_only[j] = true disables item j unless exactly one row is selected.
    void SetContextItems(const std::vector<std::string>& items,
                         const std::vector<bool>& single_only) {
        context_item_names = items;
        context_item_single_only = single_only;
        context_item_single_only.resize(items.size(), false);
    }
    void clear() {
        selected_indexes.clear();
        last_selected_index = -1;
    }

    // Returns the index of the last plain left-clicked item, then resets to -1.
    int GetJustClicked() {
        int tmp = just_clicked;
        just_clicked = -1;
        return tmp;
    }
};

#endif