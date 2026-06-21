#pragma once
#include "ObjectFactory.hpp"

namespace core {
    class LineFactory : public ObjectFactory {
        std::string name_;
        core::Point a_, b_;
        ImU32 color_;
    public:
        LineFactory(const std::string& name, core::Point a, core::Point b, ImU32 color);
        Object build() override;
    };
}
