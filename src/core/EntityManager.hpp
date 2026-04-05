/* 
    Classe invocada pela interface para construir os objetos e jogá-los no display file. 
    Apenas uma frescura para não incluir esses headers na interface, apesar de que eles serão incluidos indiretamente.
*/
#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H
#include "DisplayFile.hpp"
#include "Line.hpp"
#include "Point.hpp"
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

        std::string getName(core::ShapeType type, unsigned int fake_id){
            std::string name;
            switch(type){
                case core::ShapeType::POINT: name = "P"; break;
                case core::ShapeType::LINE: name = "L"; break;
                case core::ShapeType::WIREFRAME: name = "W"; break;
                default: name = "Notdef"; break;
            }
            name.append(std::to_string(fake_id));
            return name;
        }

        core::ShapeType getType(const std::vector<std::pair<float, float>> &p){
            if(p.size() == 1) return core::ShapeType::POINT;
            if(p.size() == 2) return core::ShapeType::LINE;
            if(p.size() >= 3) return core::ShapeType::WIREFRAME;
            return core::ShapeType::NONE;
        }

    public:
        // DisplayFile pertencerá a main
        EntityManager(DisplayFile &df): displayFile(df) {
            currentId = 1;
        }

        long long nextID(core::ShapeType type){
            long long id = this->currentId*10 + (int)type;
            this->currentId++;
            return id;
        }

        void addPoint(const std::string &name, float x, float y);
        void addLine(const std::string &name, float x1, float y1, float x2, float y2);
        void addWireframe(const std::string &name, std::vector<std::pair<float, float>> &vp);
        void add(const std::string &name, std::vector<std::pair<float, float>> &p);
        void add(const bool generate_name, std::vector<std::pair<float, float>> &p);

        const std::vector<core::Point>& getPointList() const {return displayFile.getPointList();}
        const std::vector<core::Line>& getLineList() const {return displayFile.getLineList();}
        const std::vector<core::Wireframe>& getWireframeList() const {return displayFile.getWireframeList();}
        const std::vector<ManifestEntry>& GetManifest() const { return displayFile.getManifest(); }
        const std::unordered_map<long long, std::pair<int, int>>&  getHashID() const {return displayFile.getHashID();}
        
        std::string GetObjectDetails(long long real_id, bool p3d = false) const;

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

        void ApplyTransformation(long long id, const core::Matrix<float>& matrix) {
            // IMPLEMENTAR APLICAÇÃO DE TRANSFORMAÇÃO AQUI
        }
};


#endif // ENTITYMANAGER_H
