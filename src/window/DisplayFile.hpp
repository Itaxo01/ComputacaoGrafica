#ifndef DISPLAYFILE_H
#define DISPLAYFILE_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include "Object.hpp"
#include "ObjectMetadatas/Metadata.hpp"

class DisplayFile {
private:
    std::vector<core::Object> objects;
    std::unordered_map<long long, int> hash_id;              // id → index in objects
    std::unordered_map<long long, std::unique_ptr<Metadata>> metadata;

    // Preview state
    std::vector<std::tuple<float, float, float>> preview_points;
    core::ObjectType preview_mode = core::ObjectType::NONE;
    int preview_method = 0;

public:
    void add(core::Object obj);
    void remove(long long id);
    void clear();

    const std::vector<core::Object>& getObjects() const { return objects; }
    core::Object& getObject(long long id) { return objects[hash_id.at(id)]; }
    const core::Object& getObject(long long id) const { return objects[hash_id.at(id)]; }

    template<typename T>
    T* getMetadata(long long id) {
        auto it = metadata.find(id);
        return it != metadata.end() ? static_cast<T*>(it->second.get()) : nullptr;
    }
    template<typename T>
    const T* getMetadata(long long id) const {
        auto it = metadata.find(id);
        return it != metadata.end() ? static_cast<const T*>(it->second.get()) : nullptr;
    }
    void setMetadata(long long id, std::unique_ptr<Metadata> meta) {
        metadata[id] = std::move(meta);
    }

    const std::unordered_map<long long, int>& getHashID() const { return hash_id; }

    void setPreviewState(const std::vector<std::tuple<float, float, float>>& pts,
                         core::ObjectType mode, int method = 0) {
        preview_points = pts;
        preview_mode   = mode;
        preview_method = method;
    }
    const std::vector<std::tuple<float, float, float>>& getPreviewPoints() const { return preview_points; }
    core::ObjectType getPreviewMode()   const { return preview_mode; }
    int             getPreviewMethod() const { return preview_method; }

    DisplayFile() {}
};

#endif // DISPLAYFILE_H
