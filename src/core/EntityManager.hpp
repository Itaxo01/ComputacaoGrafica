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

/* Current geometric primitives:
    Line
    Point
    Wireframe
*/
class EntityManager{
    private: 
        enum class Types{ // I intend to abuse the fact that the enum is a int. 
            LINE,
            POINT,
            WIREFRAME 
        }; // Se esse enum em algum momento passar de 10, precisaremos mudar a lógica do nextID
        long long currentId = 1;
        DisplayFile &displayFile; // EntityManager deve ser o único

    public:
        // DisplayFile pertencerá a main
        EntityManager(DisplayFile &df): displayFile(df) {
            currentId = 1;
        }

        long long nextID(Types type){
            long long id = currentId*10 + (int)type;
            currentId++;
            return id;
        }

        void addPoint(const std::string &name, float x, float y){}
        void addLine(const std::string &name, float x1, float y1, float x2, float y2){}
        void addWireframe(const std::string &name, std::vector<std::pair<float, float>>){}
        void add(const std::string &name, std::vector<std::pair<float, float>> &p){
            if(p.size() == 1) return addPoint(name, p[0].first, p[0].second);
            if(p.size() == 2) return addLine(name, p[0].first, p[0].second, p[1].first, p[1].second);
            if(p.size() >= 3) return addWireframe(name, p);
        }
};


#endif // ENTITYMANAGER_H
