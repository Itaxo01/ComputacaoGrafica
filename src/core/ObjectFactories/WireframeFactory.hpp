#pragma once
#include "ObjectFactory.hpp"

namespace core {
    class WireframeFactory : public ObjectFactory {
        std::string name_;
        std::vector<core::Point> pts_;
        ImU32 color_;
    public:
        WireframeFactory(const std::string& name, const std::vector<core::Point>& pts, ImU32 color);
        Object build() override;
    };
}
