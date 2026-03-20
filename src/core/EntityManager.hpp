/* 
    Classe invocada pela interface para construir os objetos e jogá-los no display file. 
    Apenas uma frescura para não incluir esses headers na interface, apesar de que eles serão incluidos indiretamente.
*/
#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H
#include "DisplayFile.hpp"

/* Current geometric primitives:
    Line
    Point
    Wireframe
*/
class EntityManager{
    private: 
        enum Types{ // I intend to abuse the fact that the enum is a int. 
            LINE,
            POINT,
            WIREFRAME
        };
        long long nextId = 0;
    public:
        void 
};


#endif // ENTITYMANAGER_H
