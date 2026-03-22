#ifndef SHAPETYPE_HPP
#define SHAPETYPE_HPP

enum class ShapeType {
    POINT, // 0
    LINE, // 1
    WIREFRAME, // 2
    NONE, // 3
}; // Se esse enum passar de 10 elementos, modificar a lógica de ID do entity manager

#endif // SHAPETYPE_HPP