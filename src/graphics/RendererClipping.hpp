#pragma once
#include <vector>
#include "RenderedObject.hpp"
#include "Line.hpp"

// Clip all objects in the list to the NCS window [-1, 1].
// line_clip_mode: 0 = Liang-Barsky, 1 = Cohen-Sutherland.
void ClipObjects(std::vector<RenderedObject>& objs,
                 const core::Point& wp0, const core::Point& wp1,
                 int line_clip_mode);

// Single-segment clip used by RendererBackground (keeps the core::Line interface).
bool ClipLine(core::Line& line, const core::Point& wp0, const core::Point& wp1);
