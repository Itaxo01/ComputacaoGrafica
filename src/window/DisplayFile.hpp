/*
    DisplayFile que guardará a lista de objetos do src/core a serem renderizados
*/
#ifndef DISPLAYFILE_H
#define DISPLAYFILE_H

#include <vector>

#include "Point.hpp"
#include "Line.hpp"
#include "Wireframe.hpp"
#include <unordered_map>
#include "ShapeType.hpp"

template <typename T>
inline void erase_id(std::vector<T>&v, size_t i){
    v.erase(v.begin()+i);
}

struct ManifestEntry {
    long long id; // id do objeto (Aqui haverá um truque com o tipo do objeto para melhorar a hash table)
    ShapeType type;
    std::string name;
    ManifestEntry(long long id, ShapeType type, std::string name): id(id), type(type), name(name) {}
};

/* Current geometric primitives:
    Line
    Point
    Wireframe
*/

class DisplayFile{
private:
    std::vector<core::Point> pointList;
    std::vector<core::Line> lineList;
    std::vector<core::Wireframe> wireframeList;

    std::vector<ManifestEntry> manifest; // Essa é a lista que a interface irá conhecer. 

    /*
        hash_id: a partir de um id, guarda a posição do elemento no manifest e na lista do seu tipo.
        pair.first() = ListId, pair.second() = manifestId.
    */ 
    std::unordered_map<long long, std::pair<int, int>> hash_id; 

public:
    void add(core::Point &k, const std::string &name, const long long id) {
        hash_id[id] = std::make_pair(pointList.size(), manifest.size());
        manifest.push_back(ManifestEntry(id, ShapeType::POINT, name));
        pointList.push_back(k);
    }
    void add(core::Line &k, const std::string &name, const long long id) {
        hash_id[id] = std::make_pair(lineList.size(), manifest.size());
        manifest.push_back(ManifestEntry(id, ShapeType::LINE, name));
        lineList.push_back(k);
    }
    void add(core::Wireframe &k, const std::string &name, const long long id) {
        hash_id[id] = std::make_pair(wireframeList.size(), manifest.size());
        manifest.push_back(ManifestEntry(id, ShapeType::WIREFRAME, name));
        wireframeList.push_back(k);
    }
    void remove(const long long id){
        ShapeType type = (ShapeType)(id%10);
        auto [list_id, manifest_id] = hash_id.at(id);
        erase_id(manifest, manifest_id);
        
        switch(type){
            case ShapeType::LINE:
                erase_id(lineList, list_id);
                break;
            case ShapeType::POINT:
                erase_id(pointList, list_id);
                break;
            case ShapeType::WIREFRAME:
                erase_id(wireframeList, list_id);
                break;
            default:
                break;
        }
    }

    std::vector<core::Point> getPointList() {return pointList;}
    std::vector<core::Line> getLineList() {return lineList;}
    std::vector<core::Wireframe> getWireframeList() {return wireframeList;}
    DisplayFile(){}
};


#endif // DISPLAYFILE_H