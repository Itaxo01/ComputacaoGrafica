#pragma once
#include "ObjectFactory.hpp"

namespace core {
    class PointFactory : public ObjectFactory {
        std::string name_;
        float x_, y_, z_;
        ImU32 color_;
    public:
        PointFactory(const std::string& name, float x, float y, float z, ImU32 color);
        Object build() override;
    };
}
