#ifndef RENDERER_H
#define RENDERER_H

#include "DisplayFile.hpp"
#include "Line.hpp"
#include "Point.hpp"
#include "Wireframe.hpp"

class Renderer {
private:
    DisplayFile &displayFile;

    void DrawObject(core::Point point);
public:
    Renderer(DisplayFile &df): displayFile(df) {}
    void render();
};

#endif
