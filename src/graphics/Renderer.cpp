#include "Renderer.hpp"

void Renderer::DrawObject(core::Point point) {
    // TO DO
    return;
}

void Renderer::render() {
    // TO DO
    std::vector<core::Point> pointList = displayFile.getPointList();
    for (core::Point point : pointList) {
        DrawObject(point);
    }
}