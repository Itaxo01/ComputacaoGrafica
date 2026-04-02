#ifndef RENDERER_UTILS_HPP
#define RENDERER_UTILS_HPP

#include <vector>
#include "Line.hpp"
#include "Point.hpp"
#include "Wireframe.hpp"
#include "Window.hpp"

std::vector<core::Point> ClipPoints(const std::vector<core::Point> &v, const core::Point &wp0, const core::Point &wp1);

/* Liang-Barsky clipping */
std::vector<core::Line> ClipLines(const std::vector<core::Line> &v, const core::Point &wp0, const core::Point &wp1);

/* Quebra o wireframe em linhas e faz o clipping de linha. */
std::vector<core::Line> ClipWireframes(const std::vector<core::Wireframe> &v, const core::Point &wp0, const core::Point &wp1);

void ViewportTransform(std::vector<core::Point> &points, const Window &window);
void ViewportTransform(std::vector<core::Line> &lines, const Window &window);

#endif // RENDERER_UTILS_HPP
