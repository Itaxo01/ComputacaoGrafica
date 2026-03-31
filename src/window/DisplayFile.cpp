#include "DisplayFile.hpp"
#include <stdexcept>

void DisplayFile::add(core::Shape &k, const std::string &name, const long long id) {
    int listId = -1;
    switch(k.type){
        case core::ShapeType::POINT: {
            auto &point = static_cast<core::Point&>(k);
            listId = pointList.size();
            pointList.push_back(point);
            break;
        }
        case core::ShapeType::LINE: {
            auto &line = static_cast<core::Line&>(k);
            listId = lineList.size();
            lineList.push_back(line);
            break;
        }
        case core::ShapeType::WIREFRAME: {
            auto &wireframe = static_cast<core::Wireframe&>(k);
            listId = wireframeList.size();
            wireframeList.push_back(wireframe);
            break;
        }
        default:
            throw std::runtime_error("Invalid ShapeType in DisplayFile::add");
    }
    if(listId == -1){
        throw std::logic_error("ListId not initialized in DisplayFile::add");
    }
    hash_id[id] = std::make_pair(listId, manifest.size());
    manifest.push_back(ManifestEntry(id, k.type, name));
    object_count++;
}

void DisplayFile::remove(const long long id){
    core::ShapeType type = (core::ShapeType)(id%10);
    auto [list_id, manifest_id] = hash_id.at(id);
    erase_id(manifest, manifest_id);
    
    switch(type){
        case core::ShapeType::LINE: {
            erase_id(lineList, list_id);
            break;
        }
        case core::ShapeType::POINT: {
            erase_id(pointList, list_id);
            break;
        }
        case core::ShapeType::WIREFRAME: {
            erase_id(wireframeList, list_id);
            break;
        }
        default:
            throw std::runtime_error("Invalid object at display file remove");
    }
    object_count--;
}