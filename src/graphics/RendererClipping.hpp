#pragma once
#include <vector>
#include "GeometryBuffer.hpp"
#include "Line.hpp"

// Near-plane clip in VRC (camera) space, run before the perspective divide. Keeps
// geometry with z >= near_z (in front of the COP); trims line segments that cross
// the plane and drops points/triangles behind it. Returns a fresh EXPANDED buffer
// (each surviving primitive owns its own vertices). This prevents w = z + d from
// reaching <= 0, which would wrap vertices to the wrong side during the divide.
GeometryBuffer NearClipFlat(const GeometryBuffer& in, float near_z);

// Clip everything to the NCS window [wp0, wp1]. Filled polygons/meshes get their
// triangles Sutherland-Hodgman clipped + re-triangulated (shading attributes
// interpolated at every cut); their outlines and all other lines are segment
// clipped (Liang-Barsky, or Cohen-Sutherland when line_clip_mode == 1 for
// non-filled objects); points are kept iff inside. Returns a fresh EXPANDED buffer.
// `slices` is read for each primitive's filled/type flags.
GeometryBuffer BoxClipFlat(const GeometryBuffer& in, const std::vector<ObjectSlice>& slices,
                           const core::Point& wp0, const core::Point& wp1, int line_clip_mode);

// Single-segment clip used by RendererBackground (keeps the core::Line interface).
bool ClipLine(core::Line& line, const core::Point& wp0, const core::Point& wp1);
