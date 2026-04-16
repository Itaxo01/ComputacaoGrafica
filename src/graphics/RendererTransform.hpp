#pragma once
#include <vector>
#include "Line.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "Wireframe.hpp"
#include "Window.hpp"
#include "Mat4.hpp"

void TransformToNCS(std::vector<core::Point> &points, const core::mat4 &ncs_mat);
void TransformToNCS(std::vector<core::Line> &lines, const core::mat4 &ncs_mat);
void TransformToNCS(std::vector<core::Wireframe> &wireframes, const core::mat4 &ncs_mat);
void TransformToNCS(std::vector<core::Polygon> &polygons, const core::mat4 &ncs_mat);

void TransformToViewport(std::vector<core::Point> &points, const Window &window, const ImVec2 &offset);
void TransformToViewport(std::vector<core::Line> &lines, const Window &window, const ImVec2 &offset);
void TransformToViewport(std::vector<core::Polygon> &polygon, const Window &window, const ImVec2 &offset);
