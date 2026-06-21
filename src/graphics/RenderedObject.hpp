#pragma once
#include <vector>
#include <utility>
#include <tuple>
#include <cstdint>
#include "Object.hpp" // for core::ObjectType
#include "Point.hpp"
#include "imgui.h"

// Render-only view of a mesh: just the geometry the rasterizer touches. The full
// core::Mesh (texcoords, normals, faces, per-mesh materials, ...) is storage-side
// only and is deliberately NOT copied here — this keeps the per-rebuild transform
// pipeline lean now that core::Mesh carries full OBJ fidelity.
struct RenderMesh {
    std::vector<core::Point> vertices;
    std::vector<std::pair<uint32_t, uint32_t>> line_indices;
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> tri_indices;
};

struct RenderedObject {
    core::ObjectType type   = core::ObjectType::NONE;
    ImU32            color  = 0xFF;   // flat draw color (from Material::color)
    bool             filled = false;  // from Material::filled (used by clipping)
    RenderMesh       mesh;
};

// A single screen-space triangle ready to draw. Triangles from every object are
// gathered into one list so they can be depth-sorted globally (painter's
// algorithm) for correct cross-object occlusion — see BuildSortedTriangles.
struct SortedTri {
    ImVec2 a, b, c;
    ImU32  color;
    float  depth;        // NCS-space average z, used by the painter's sort
    float  za, zb, zc;   // per-vertex NCS depth, interpolated per pixel by the z-buffer
};
