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

// Near-plane clip in VRC (camera) space, run before the perspective divide.
// Keeps geometry with z >= near_z (in front of the COP); trims line segments
// that cross the plane and drops points/triangles behind it. This prevents
// w = z + d from reaching <= 0, which would wrap vertices to the wrong side
// of the screen during the divide.
void ClipNearPlane(std::vector<RenderedObject>& objs, float near_z);
