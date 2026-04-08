// Classe de objeto para polimorfismo
#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <string>
#include <tuple>
#include <utility>
#include "Util.hpp"
#include "imgui.h"
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
            virtual std::tuple<float, float, float> anchorPoint() const = 0;
            virtual std::tuple<float, float, float> centerPoint() const = 0;
            
            std::string getName() const{
                #ifndef DONT_DRAW_SHAPE_NAME
                    return name;
                #endif
                return "idk";
            }
            
            std::string getColor() const{ // r g b
                #ifndef DONT_USE_OBJECT_COLOR
                    unsigned int mask = -1;
                    mask >>= 24;
                    int object_color_copy = object_color;
                    int r = object_color_copy & mask; object_color_copy >>= 8;
                    int g = object_color_copy & mask; object_color_copy >>= 8;
                    int b = object_color_copy & mask; object_color_copy >>= 8;
                    std::string res = "[" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + "]";
                    return res;
                #endif
                return "idk";
            } 

            #ifndef DONT_DRAW_SHAPE_NAME // Use to load the shape name on the interface Viewport
                std::string name;
            #endif
            #ifndef DONT_USE_OBJECT_COLOR
                int object_color; // Uses 32 bit 
            #endif
    };
}




#endif // SHAPE_HPP