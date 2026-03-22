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
#include "Shape.hpp"

template <typename T>
inline void erase_id(std::vector<T>&v, size_t i){
    v.erase(v.begin()+i);
}

struct ManifestEntry {
    long long id; // id do objeto (Aqui haverá um truque com o tipo do objeto para melhorar a hash table)
    core::ShapeType type;
    std::string name;
    ManifestEntry(long long id, core::ShapeType type, std::string name): id(id), type(type), name(name) {}
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
    void add(core::Shape &k, const std::string &name, const long long id);
    void remove(const long long id);

    std::vector<core::Point> getPointList() {return pointList;}
    std::vector<core::Line> getLineList() {return lineList;}
    std::vector<core::Wireframe> getWireframeList() {return wireframeList;}
    DisplayFile(){}
};


#endif // DISPLAYFILE_H