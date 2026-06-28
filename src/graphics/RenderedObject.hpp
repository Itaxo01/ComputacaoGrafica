#pragma once
#include "Point.hpp"
#include "Shading.hpp" // ShadeMaterial
#include "imgui.h"

// A single screen-space triangle ready to draw. Triangles from every object are
// gathered into one list (see BuildSortedTrianglesFlat) so they can be depth-sorted
// globally (painter's algorithm) for correct cross-object occlusion, or fed to the
// per-pixel z-buffer.
//
// (The old per-object RenderMesh / RenderedObject views are gone: geometry now
// lives in a flat GeometryBuffer — see GeometryBuffer.hpp.)
struct SortedTri {
    ImVec2 a, b, c;
    ImU32  color;
    float  depth;        // NCS-space average z, used by the painter's sort
    float  za, zb, zc;   // per-vertex NCS depth, interpolated per pixel by the z-buffer
    // Shading attributes (valid when the triangle came from a shaded object):
    core::Point  P[3];   // per-vertex world position
    core::Point  N[3];   // per-vertex world normal
    ShadeMaterial mat;   // material reflectivities
};
