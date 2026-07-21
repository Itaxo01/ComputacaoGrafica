#pragma once
#include <vector>
#include "Point.hpp"
#include "ObjectMetadatas/Metadata.hpp"
#include "ObjectMetadatas/CurveMetadata.hpp"   // BEZIER / BSPLINE

// Tessellation technique. The two are mathematically equivalent (forward
// differences just evaluates the same blending polynomial incrementally), so the
// choice only selects which algorithm runs to produce the sample grid.
#define SURF_BLENDING     0    // patches + blending functions (direct S·M·G·Mᵀ·Tᵀ)
#define SURF_FORWARD_DIFF 1    // forward differences (E·C·Eᵀ, accumulated)

// A bicubic surface is defined by an M×N control grid (M = rows, N = cols, both
// >= 4), stored row-major in control_points (control_points[r*cols + c]). The
// method (BEZIER / BSPLINE) selects both the basis matrix and how the grid is
// split into overlapping/composite 4x4 patches when tessellating.
struct SurfaceMetadata : public Metadata {
    int rows = 0;
    int cols = 0;
    std::vector<core::Point> control_points;  // row-major rows*cols control grid
    int method     = BEZIER;
    int technique  = SURF_FORWARD_DIFF;  // SURF_BLENDING / SURF_FORWARD_DIFF
    int resolution = 12;        // samples per patch edge (grid is resolution x resolution)
    bool filled    = false;     // solid (tessellated triangles) vs. iso-curve wireframe
};
