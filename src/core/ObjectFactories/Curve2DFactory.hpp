#pragma once
#include "ObjectFactory.hpp"
#include "ObjectMetadatas/CurveMetadata.hpp"
#include <memory>

namespace core {
    class Curve2DFactory : public ObjectFactory {
        std::string name_;
        std::vector<core::Point> control_pts_;
        int smoothness_;
        int method_;
        ImU32 color_;
        CurveMetadata meta_;
    public:
        Curve2DFactory(const std::string& name, const std::vector<core::Point>& control_pts,
                       int smoothness, int method, ImU32 color);
        Object build() override;
        std::unique_ptr<Metadata> takeMetadata() override;
    };
}
