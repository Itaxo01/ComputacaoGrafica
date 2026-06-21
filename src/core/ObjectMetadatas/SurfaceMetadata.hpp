#pragma once
#include <vector>
#include "Point.hpp"
#include "ObjectMetadatas/Metadata.hpp"
#include "ObjectMetadatas/CurveMetadata.hpp"   // BEZIER / BSPLINE

// A bicubic surface is a list of patches ("retalhos"); each patch is a 4x4 grid
// of 16 control points stored row-major (control[row*4 + col]). The method
// (BEZIER / BSPLINE) selects the basis matrix used to tessellate every patch.
struct SurfaceMetadata : public Metadata {
    std::vector<std::vector<core::Point>> patches;  // each inner vector holds 16 control points
    int method     = BEZIER;
    int resolution = 12;        // samples per patch edge (grid is resolution x resolution)
};
