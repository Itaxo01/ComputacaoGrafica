#pragma once
#include <vector>
#include "Point.hpp"
#include "ObjectMetadatas/Metadata.hpp"

#define BEZIER  0
#define BSPLINE 1

struct CurveMetadata : public Metadata {
    std::vector<core::Point> control_points;
    int smoothness = 50;
    int method = BEZIER;
};
