#include "DisplayFile.hpp"
#include <stdexcept>

void DisplayFile::add(core::Object obj) {
    long long id = obj.id;
    hash_id[id] = (int)objects.size();
    objects.push_back(std::move(obj));
}

void DisplayFile::remove(long long id) {
    auto it = hash_id.find(id);
    if (it == hash_id.end()) return;

    int idx = it->second;

    // Ordered erase preserves insertion order for the UI.
    // Fix up hash entries for every element that shifts left.
    objects.erase(objects.begin() + idx);
    hash_id.erase(id);
    for (int i = idx; i < (int)objects.size(); i++)
        hash_id[objects[i].id] = i;

    metadata.erase(id);
}

void DisplayFile::clear() {
    objects.clear();
    hash_id.clear();
    metadata.clear();
}
