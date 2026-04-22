/* 
    Classe invocada pela interface para construir os objetos e jogá-los no display file. 
    Apenas uma frescura para não incluir esses headers na interface, apesar de que eles serão incluidos indiretamente.
*/
#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H
#include "DisplayFile.hpp"
#include "Line.hpp"
#include "Point.hpp"
#include "Renderer.hpp"
#include "Wireframe.hpp"
#include "Shape.hpp"
#include <set>
#include <iostream>
#include <string>
#include <unordered_map>

/* Current geometric primitives:
    Line
    Point
    Wireframe
*/
class EntityManager{
    private: 
        long long currentId = 1;
        DisplayFile &displayFile; // EntityManager deve ser o único
        Renderer &renderer;

        std::string getName(core::ShapeType type, unsigned int fake_id){
            std::string name;
            switch(type){
                case core::ShapeType::POINT: name = "P"; break;
                case core::ShapeType::LINE: name = "L"; break;
                case core::ShapeType::WIREFRAME: name = "W"; break;
                case core::ShapeType::POLYGON: name = "POLY"; break;
                case core::ShapeType::CURVE2D: name = "C2D"; break;
                default: name = "Notdef"; break;
            }
            name.append(std::to_string(fake_id));
            return name;
        }

    public:
        // DisplayFile pertencerá a main
        EntityManager(DisplayFile &df, Renderer &r): displayFile(df), renderer(r) {
            currentId = 1;
        }

        long long nextID(core::ShapeType type){
            long long id = this->currentId*10 + (int)type;
            this->currentId++;
            return id;
        }

        void addPoint(const std::string &name, std::tuple<float, float, float> &t, int object_color);
        void addLine(const std::string &name, std::tuple<float, float, float> &t1, std::tuple<float, float, float> &t2, int object_color);
        void addWireframe(const std::string &name, std::vector<std::tuple<float, float, float>> &vp, int object_color);
        void addPolygon(const std::string &name, std::vector<std::tuple<float, float, float>> &vp, bool filled, int object_color);
        void addCurve2D(const std::string &name, std::vector<std::tuple<float, float, float>> &vp, int object_color, int smoothness = 50);
        void add(const std::string &name, std::vector<std::tuple<float, float, float>> &p, core::ShapeType &type, bool filled, int object_color, int smoothness = 50);
        void add(const bool generate_name, std::vector<std::tuple<float, float, float>> &p, core::ShapeType &type, bool filled, int object_color, int smoothness = 50);

        const std::vector<core::Point>& getPointList() const {return displayFile.getPointList();}
        const std::vector<core::Line>& getLineList() const {return displayFile.getLineList();}
        const std::vector<core::Wireframe>& getWireframeList() const {return displayFile.getWireframeList();}
        const std::vector<core::Polygon>& getPolygonList() const {return displayFile.getPolygonList();}
        const std::vector<core::Curve2D>& getCurve2DList() const {return displayFile.getCurve2DList();}
        const std::vector<ManifestEntry>& GetManifest() const { return displayFile.getManifest(); }
        const std::unordered_map<long long, std::pair<int, int>>&  getHashID() const {return displayFile.getHashID();}
        
        core::ObjectDetails GetObjectDetails(long long real_id, bool p3d = false) const;

        std::vector<std::string> GetObjectNames() const {
            std::vector<std::string> names;
            for(const auto &entry: GetManifest()){
                names.push_back(entry.name);
            }
            return names;
        }

        std::vector<long long> GetObjectIDs() const {
            std::vector<long long> ids;
            for(const auto &entry: GetManifest()){
                ids.push_back(entry.id);
            }
            return ids;
        }

        size_t GetObjectCount() const {
            return GetManifest().size();
        }

        void remove(long long id) {
            displayFile.remove(id);
        }

        void setPreviewState(const std::vector<std::tuple<float, float, float>>& pts, core::ShapeType mode) {
            displayFile.setPreviewState(pts, mode);
        }

        core::Shape& getObject(long long id) {
            return displayFile.getShape(id);
        }

        void ApplyTransformation(long long real_id, const core::mat4& matrix);
};


#endif // ENTITYMANAGER_H
