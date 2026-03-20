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

enum class ShapeType{POINT, LINE, WIREFRAME};

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
    std::vector<core::Point> pointList;
    std::vector<core::Line> lineList;
    std::vector<core::Wireframe> wireframeList;

    std::vector<ManifestEntry> manifest; // Essa é a lista que a interface irá conhecer. 

    std::unordered_map<long long, int> id_hash;

    DisplayFile(){}

    void add(core::Point &k, const std::string &name, const long long id) {
        manifest.push_back(ManifestEntry(id, ShapeType::POINT, name));
        pointList.push_back(k);
    }
    void add(core::Line &k, const std::string &name, const long long id) {
        lineList.push_back(k);
    }
    void add(core::Wireframe &k, const std::string &name, const long long id) {
        wireframeList.push_back(k);
    }
};


#endif // DISPLAYFILE_H