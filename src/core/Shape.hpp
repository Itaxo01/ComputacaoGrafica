// Classe de objeto para polimorfismo
#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <utility>
#include "Util.hpp"
namespace core{
    enum class ShapeType {
        POINT, // 0
        LINE, // 1
        WIREFRAME, // 2
        NONE, // 3
        ENUM_SIZE, // 4
    }; // Se esse enum passar de 10 elementos, modificar a lógica de ID do entity manager

    
    class Shape{
        public:
            ShapeType type;
            virtual ~Shape() = default;
            virtual std::pair<float, float> anchorPoint() const = 0;

            #ifndef DONT_DRAW_SHAPE_NAME // Use to load the shape name on the interface Viewport
                std::string name;
            #endif
    };
}

#endif // SHAPE_HPP