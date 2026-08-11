#pragma once
#include <vector>
#include <utility>
#include <tuple>
#include <cstdint>
#include "Object.hpp" // for core::ObjectType
#include "Point.hpp"
#include "Shading.hpp" // ShadeMaterial
#include "imgui.h"

// Render-only view of a mesh: just the geometry the rasterizer touches. The full
// core::Mesh (texcoords, normals, faces, per-mesh materials, ...) is storage-side
// only and is deliberately NOT copied here — this keeps the per-rebuild transform
// pipeline lean now that core::Mesh carries full OBJ fidelity.
//
// world_vertices / world_normals are populated ONLY when shading is on (they feed
// the Phong lighting), and are carried/interpolated through clipping alongside the
// working positions so they stay index-aligned with `vertices`.
struct RenderMesh {
    std::vector<core::Point> vertices;
    std::vector<std::pair<uint32_t, uint32_t>> line_indices;
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> tri_indices;

    std::vector<core::Point> world_vertices; // world-space positions (shading)
    std::vector<core::Point> world_normals;  // smooth world-space normals (shading)
};

struct RenderedObject {
    core::ObjectType type   = core::ObjectType::NONE;
    ImU32            color  = 0xFF;   // flat draw color (from Material::color)
    bool             filled = false;  // from Material::filled (used by clipping)
    ShadeMaterial    shadeMat;        // ka/kd/ks/shininess (populated when shading on)
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
    // Shading attributes (valid when the triangle came from a shaded object):
    core::Point  P[3];   // per-vertex world position
    core::Point  N[3];   // per-vertex world normal
    ShadeMaterial mat;    // material reflectivities
};

// A SortedTri's pixel bounding box, in a SEPARATE array from sortedTris itself.
// The banded rasterizer scans every triangle in every band to find the ones it
// owns; walking a 16-byte record instead of the 156-byte SortedTri keeps that
// scan ~10x smaller (at 300k triangles: ~5 MB streamed per band instead of
// ~47 MB, against 4 MB of L3 per CCX). Computed once in BuildSortedTriangles,
// so the rasterizer also stops redoing the same floor/ceil/min/max per band.
struct TriBounds {
    int min_x, min_y, max_x, max_y;
};
