#pragma once
#include "imgui.h"
#include "Window.hpp"
#include "Mat4.hpp"
#include <vector>
#include <tuple>

using PreviewPts = std::vector<std::tuple<float, float, float>>;

void DrawPreviewPolyline(ImDrawList *dl, const PreviewPts &pts, const core::mat4 &ncs_mat, const Window &window, ImVec2 offset);
void DrawPreviewPolygon (ImDrawList *dl, const PreviewPts &pts, const core::mat4 &ncs_mat, const Window &window, ImVec2 offset);
void DrawPreviewCurve2D  (ImDrawList *dl, const PreviewPts &pts, const core::mat4 &ncs_mat, const Window &window, ImVec2 offset);
