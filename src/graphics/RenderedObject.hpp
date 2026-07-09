#pragma once
#include "Point.hpp"
#include "Shading.hpp" // ShadeMaterial
#include "HostDevice.hpp"
#include "imgui.h"

// A single screen-space triangle ready to draw. Triangles from every object are
// gathered into one list (see buildSortedTriangle in PipelineStages.hpp) so they can
// be depth-sorted globally (painter's) or fed to the per-pixel z-buffer.
//
// a/b/c are viewport-space positions (z unused there). Using core::Point (not ImVec2)
// keeps SortedTri trivially constructible in device code, so the SAME build/raster
// code runs on the CPU and in CUDA kernels.
struct SortedTri {
    core::Point a, b, c;   // viewport-space vertices (x,y; z ignored)
    ImU32  color;
    float  depth;          // NCS-space average z, used by the painter's sort
    float  za, zb, zc;     // per-vertex NCS depth, interpolated per pixel by the z-buffer
    core::Point  P[3];     // per-vertex world position (shading)
    core::Point  N[3];     // per-vertex world normal (shading)
    ShadeMaterial mat;     // material reflectivities
};
