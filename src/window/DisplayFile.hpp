/*
    DisplayFile que guardará a lista de objetos do src/core a serem renderizados
*/
#ifndef DISPLAYFILE_H
#define DISPLAYFILE_H

#include <vector>

#include "Point.hpp"
#include "Line.hpp"
#include "Polygon.hpp"
#include "Wireframe.hpp"
#include <unordered_map>
#include "Shape.hpp"

template <typename T>
inline void erase_id(std::vector<T>&v, size_t i){
    v.erase(v.begin()+i);
}

struct ManifestEntry {
    long long id;
    long long fake_id; // fake id não contém o id de identificador do tipo do objeto
    core::ShapeType type;
    std::string name;
    ManifestEntry(long long id, core::ShapeType type, std::string name): id(id), type(type), name(name) {
        fake_id = id/10;
    }
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
    std::vector<core::Polygon> polygonList;

    std::vector<ManifestEntry> manifest; // Essa é a lista que a interface irá conhecer. 

    /*
        hash_id: a partir de um id, guarda a posição do elemento no manifest e na lista do seu tipo.
        pair.first() = ListId, pair.second() = manifestId.
    */ 
    std::unordered_map<long long, std::pair<int, int>> hash_id; 

    // Preview state: in-progress object being created by the user
    std::vector<std::tuple<float, float, float>> preview_points;
    core::ShapeType preview_mode = core::ShapeType::NONE;

public:
    unsigned long object_count = 0;
    void add(core::Shape &k, const std::string &name, const long long id);
    void remove(const long long id);

    void setPreviewState(const std::vector<std::tuple<float, float, float>>& pts, core::ShapeType mode) {
        preview_points = pts;
        preview_mode   = mode;
    }
    const std::vector<std::tuple<float, float, float>>& getPreviewPoints() const { return preview_points; }
    core::ShapeType getPreviewMode() const { return preview_mode; }
    
    /* Não desenhamos essas, apenas armazenamos os valores */
    const std::vector<core::Point>& getPointList() const {return pointList;}
    const std::vector<core::Line>& getLineList() const {return lineList;}
    const std::vector<core::Wireframe>& getWireframeList() const {return wireframeList;}
    const std::vector<core::Polygon>& getPolygonList() const {return polygonList;}
    const std::vector<ManifestEntry>& getManifest() const {return manifest;}
    core::Shape &getShape(long long real_id);

    const core::Point &getPoint(long long id) const {return pointList[id];}
    const core::Line &getLine(long long id) const {return lineList[id];}
    const core::Wireframe &getWireframe(long long id) const {return wireframeList[id];}
    const core::Polygon &getPolygon(long long id) const {return polygonList[id];}

    const std::unordered_map<long long, std::pair<int, int>>& getHashID() const {return hash_id;}

    DisplayFile(){}
};


#endif // DISPLAYFILE_H