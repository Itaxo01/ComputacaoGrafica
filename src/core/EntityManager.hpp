#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include "DisplayFile.hpp"
#include "Object.hpp"
#include "Renderer.hpp"
#include "ObjectMetadatas/CurveMetadata.hpp"
#include "ObjectFactories/ObjectFactory.hpp"
#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

class EntityManager {
private:
    long long currentId = 1;
    DisplayFile& displayFile;
    Renderer&    renderer;

    long long nextID() { return currentId++; }

    std::string autoName(core::ObjectType type, long long id) {
        std::string prefix;
        switch (type) {
            case core::ObjectType::POINT:    prefix = "P";    break;
            case core::ObjectType::LINE:     prefix = "L";    break;
            case core::ObjectType::WIREFRAME:prefix = "W";    break;
            case core::ObjectType::POLYGON:  prefix = "POLY"; break;
            case core::ObjectType::CURVE2D:  prefix = "C2D";  break;
            default:                        prefix = "Obj";  break;
        }
        return prefix + std::to_string(id);
    }

public:
    EntityManager(DisplayFile& df, Renderer& r) : displayFile(df), renderer(r) {}

    long long add(core::ObjectFactory& factory);

    const std::unordered_map<long long, int>&        getHashID()   const { return displayFile.getHashID(); }
    const std::vector<core::Object>&                 getObjects()  const { return displayFile.getObjects(); }

    core::Object& getObject(long long id) { return displayFile.getObject(id); }

    const CurveMetadata* getCurveMetadata(long long id) const {
        return displayFile.getMetadata<CurveMetadata>(id);
    }

    core::ObjectDetails GetObjectDetails(long long id) const;
    std::vector<std::tuple<float,float,float>> GetObjectRawPoints(long long id) const;
    void UpdateObjectPoints(long long id,
                            const std::vector<std::tuple<float,float,float>>& new_pts,
                            core::ObjectType new_type, int new_method, bool new_filled);

    void remove(long long id) {
        displayFile.remove(id);
        renderer.notifyTransformation();
    }

    void removeAll() {
        displayFile.clear();
        renderer.notifyTransformation();
    }

    void setPreviewState(const std::vector<std::tuple<float,float,float>>& pts,
                         core::ObjectType mode, int method = 0) {
        displayFile.setPreviewState(pts, mode, method);
    }

    void ApplyTransformation(long long id, const core::mat4& matrix);

    std::vector<std::string> GetObjectNames() const {
        std::vector<std::string> names;
        for (const auto& obj : getObjects()) names.push_back(obj.name);
        return names;
    }
    std::vector<long long> GetObjectIDs() const {
        std::vector<long long> ids;
        for (const auto& obj : getObjects()) ids.push_back(obj.id);
        return ids;
    }
    size_t GetObjectCount() const { return getObjects().size(); }
};

#endif // ENTITYMANAGER_H
