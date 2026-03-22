// Classe de objeto para polimorfismo
#ifndef SHAPE_HPP
#define SHAPE_HPP

namespace core{
    enum class ShapeType {
        POINT, // 0
        LINE, // 1
        WIREFRAME, // 2
        NONE, // 3
    }; // Se esse enum passar de 10 elementos, modificar a lógica de ID do entity manager

    
    class Shape{
        public:
            ShapeType type;
            virtual ~Shape() = default;
    };
}

#endif // SHAPE_HPP