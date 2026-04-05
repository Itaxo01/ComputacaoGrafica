#ifndef RENDERER_UTILS_HPP
#define RENDERER_UTILS_HPP

#include <vector>
#include "Line.hpp"
#include "Point.hpp"
#include "Wireframe.hpp"
#include "Window.hpp"

std::vector<core::Point> ClipPoints(const std::vector<core::Point> &v, const core::Point &wp0, const core::Point &wp1);

std::pair<core::Line, bool> ClipLine(const core::Line &line, const core::Point &wp0, const core::Point&wp1);

/* Liang-Barsky clipping */
std::vector<core::Line> ClipLines(const std::vector<core::Line> &v, const core::Point &wp0, const core::Point &wp1);

/* Quebra o wireframe em linhas e faz o clipping de linha. */
std::vector<core::Line> ClipWireframes(const std::vector<core::Wireframe> &v, const core::Point &wp0, const core::Point &wp1);

// Paraleliza as transformações de matrizes e viewport
void TransformToNCS(std::vector<core::Point> &points, const core::Matrix<float> &ncs_mat);
void TransformToNCS(std::vector<core::Line> &lines, const core::Matrix<float> &ncs_mat);
void TransformToNCS(std::vector<core::Wireframe> &wireframes, const core::Matrix<float> &ncs_mat);

void TransformToViewport(std::vector<core::Point> &points, const Window &window, const ImVec2 &offset);
void TransformToViewport(std::vector<core::Line> &lines, const Window &window, const ImVec2 &offset);

#endif // RENDERER_UTILS_HPP
