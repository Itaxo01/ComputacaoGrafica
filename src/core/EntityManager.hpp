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

// Essa classe aqui está bem confusa e não pretendo melhorar ela, cumpre seu papel
class NameGenerator{
    std::vector<std::set<unsigned int>> freeIDs;
    std::unordered_map<unsigned int, unsigned int> id_map; // mapeia o real_id para o id do nome

    public:
        NameGenerator(unsigned int maxID = 16384, core::ShapeType enum_size = core::ShapeType::ENUM_SIZE){
            freeIDs.resize((unsigned long)enum_size);
            for(auto &s: freeIDs){
                for(unsigned int i = 1; i <= maxID; i++){
                    s.insert(i);
                }
            }
        }
        
        std::string getName(core::ShapeType type, unsigned int fake_id){
            unsigned int real_id = fake_id*10 + (unsigned int)type;
            std::string name;
            switch(type){
                case core::ShapeType::POINT:{
                    name = "P";
                    break;
                }
                case core::ShapeType::LINE:{
                    name = "L";
                    break;
                }
                case core::ShapeType::WIREFRAME:{
                    name = "W";
                    break;
                }
                default:
                    name = "Notdef";
                    break;
            }
            auto id = freeIDs[(unsigned long)type].begin();
            freeIDs[(unsigned long)type].erase(id);
            name.append(std::to_string(*id));
            id_map[real_id] = *id;
            return name;
        }

        void releaseID(unsigned int real_id){
            int type = (real_id%10);
            freeIDs[type].insert(id_map[real_id]);
        }
};
/* Current geometric primitives:
    Line
    Point
    Wireframe
*/
class EntityManager{
    private: 
        long long currentId = 1;
        DisplayFile &displayFile; // EntityManager deve ser o único
        NameGenerator nameGenerator;

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

        const std::vector<ManifestEntry>& GetManifest() const { return displayFile.getManifest(); }
        void remove(long long id) {
            std::cout<<"Deletion on object "<<id<<" type "<<(id%10)<<std::endl;
            displayFile.remove(id);
            nameGenerator.releaseID(id);
        }
};


#endif // ENTITYMANAGER_H
