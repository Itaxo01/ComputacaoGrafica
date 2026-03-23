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

/* Current geometric primitives:
    Line
    Point
    Wireframe
*/
class EntityManager{
    private: 
        long long currentId = 1;
        DisplayFile &displayFile; // EntityManager deve ser o único

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
};


#endif // ENTITYMANAGER_H
