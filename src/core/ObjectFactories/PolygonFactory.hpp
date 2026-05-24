#pragma once
#include "ObjectFactory.hpp"

namespace core {
    class PolygonFactory : public ObjectFactory {
        std::string name_;
        std::vector<core::Point> pts_;
        bool filled_;
        ImU32 color_;
    public:
        PolygonFactory(const std::string& name, const std::vector<core::Point>& pts, bool filled, ImU32 color);
        Object build() override;
    };
}
