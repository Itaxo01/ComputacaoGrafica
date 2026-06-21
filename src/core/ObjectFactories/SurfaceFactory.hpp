#pragma once
#include "ObjectFactory.hpp"
#include "ObjectMetadatas/SurfaceMetadata.hpp"
#include <memory>
#include <vector>

namespace core {
    // Builds a bicubic surface object from a list of 16-point patches. Each patch
    // is tessellated into a resolution x resolution grid of points (evaluated via
    // Q(s,t) = S·M·G·Mᵀ·Tᵀ) and emitted as iso-curve grid lines, so the surface
    // renders through the existing line pipeline. method = BEZIER or BSPLINE only
    // swaps the basis matrix M.
    class SurfaceFactory : public ObjectFactory {
        std::string name_;
        std::vector<std::vector<core::Point>> patches_;  // each holds 16 control points
        int method_;
        int resolution_;
        ImU32 color_;
        SurfaceMetadata meta_;
    public:
        SurfaceFactory(const std::string& name,
                       const std::vector<std::vector<core::Point>>& patches,
                       int method, int resolution, ImU32 color);
        Object build() override;
        std::unique_ptr<Metadata> takeMetadata() override;
    };
}
