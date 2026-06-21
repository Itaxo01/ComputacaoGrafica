#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <string>
#include <tuple>

namespace core {
    struct {
        std::string type  = "";
        std::string id    = "";
        std::string name  = "";
        std::string color = "";
        std::string points = "";
    } typedef ObjectDetails;

    enum class ObjectType {
        POINT,    // 0
        LINE,     // 1
        WIREFRAME,// 2
        NONE,     // 3
        POLYGON,  // 4
        CURVE2D,  // 5
        ENUM_SIZE,// 6
    };

    const char* getTypeName(ObjectType type);
}

#endif // SHAPE_HPP
