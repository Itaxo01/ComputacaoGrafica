#pragma once
#include "ObjectFactory.hpp"
#include "ObjectMetadatas/SurfaceMetadata.hpp"
#include <memory>
#include <vector>

namespace core {
    // Splits an M×N control grid (row-major, M = rows, N = cols, both >= 4) into
    // the 16-control-point bicubic patches that make up the surface:
    //   B-Spline: overlapping windows sliding by 1  -> (M-3)*(N-3) patches
    //   Bezier:   composite windows sliding by 3     -> ((M-1)/3)*((N-1)/3) patches
    // Each patch is row-major (cp[i*4 + j], i along s/rows, j along t/cols).
    std::vector<std::vector<core::Point>>
    surfaceGridToPatches(int rows, int cols,
                         const std::vector<core::Point>& grid, int method);

    // Builds a bicubic surface (Bezier or composite B-Spline) from an M×N control
    // grid. Every patch is tessellated into a resolution x resolution grid of
    // points; `filled` then selects how that grid is emitted:
    //   false -> iso-curve grid lines, drawn by the line pipeline (wireframe look)
    //   true  -> two triangles per grid cell, drawn solid by the rasterizer and
    //            lit by the shading model (the grid lines are dropped, since a
    //            wireframe coincident with the fill would z-fight against it)
    // method (BEZIER / BSPLINE) selects both the basis matrix and the patch
    // decomposition; technique (SURF_BLENDING / SURF_FORWARD_DIFF) selects how
    // each patch's sample grid is evaluated.
    class SurfaceFactory : public ObjectFactory {
        std::string name_;
        int rows_;
        int cols_;
        std::vector<core::Point> grid_;  // row-major rows_*cols_ control points
        int method_;
        int technique_;
        int resolution_;
        bool filled_;
        ImU32 color_;
        SurfaceMetadata meta_;
    public:
        SurfaceFactory(const std::string& name, int rows, int cols,
                       const std::vector<core::Point>& grid,
                       int method, int technique, int resolution, bool filled, ImU32 color);
        Object build() override;
        std::unique_ptr<Metadata> takeMetadata() override;
    };
}
