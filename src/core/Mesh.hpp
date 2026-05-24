#pragma once
#include <vector>
#include <utility>
#include <tuple>
#include <cstdint>
#include "Point.hpp"

namespace core {
    struct Mesh {
        std::vector<core::Point> vertices;
        std::vector<std::pair<uint32_t, uint32_t>> line_indices;
        std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> tri_indices;
    };
}
