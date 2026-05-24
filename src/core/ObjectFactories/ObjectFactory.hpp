#pragma once
#include <memory>
#include "Object.hpp"
#include "ObjectMetadatas/Metadata.hpp"

namespace core {
    class ObjectFactory {
    public:
        virtual ~ObjectFactory() = default;
        virtual Object build() = 0;
        virtual std::unique_ptr<Metadata> takeMetadata() { return nullptr; }
    };
}
