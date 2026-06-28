#pragma once
#include <vector>
#include "Object.hpp"
#include "GeometryBuffer.hpp"
#include "RenderedObject.hpp" // SortedTri
#include "Window.hpp"
#include "Mat4.hpp"

// Flatten the display-file objects into a single SoA GeometryBuffer (the "indexed"
// shape) and fill the ObjectSlice table. `pos` is left in MODEL space here; the
// transform stage bakes the matrices in. When `shading` is on, world/normal arrays
// are allocated so the transform stage can populate them.
void BuildGeometryBuffer(const std::vector<core::Object>& src,
                         GeometryBuffer& gb, std::vector<ObjectSlice>& slices, bool shading);

// Per-vertex map over the whole pool: world = transform * model (shading only),
// pos = ncs_mat * (world or model). Each object's matrix is fetched via gb.vobj[i].
// When shading is on, also builds smooth world-space vertex normals (area-weighted
// face-normal scatter over the triangles, then normalize). Pure per-vertex work =
// the exact shape of the CUDA kernel.
void TransformFlat(GeometryBuffer& gb, const std::vector<ObjectSlice>& slices,
                   const core::mat4& ncs_mat, bool shading);

// Per-vertex map: pos = mat * pos (the divide-by-w happens inside mat4 * Point).
// Perspective path only, to project VRC vertices to NCS after the near-plane clip.
void ProjectFlat(GeometryBuffer& gb, const core::mat4& mat);

// Maps the point/line vertices from NCS to framebuffer pixel space (NCSToViewport
// then * scale, keeping z for depth-testing). Triangle vertices are intentionally
// skipped — they are captured by BuildSortedTrianglesFlat in NCS space first.
// Relies on the expanded clip-output layout [point verts][line verts][tri verts].
void ViewportFlat(GeometryBuffer& gb, const Window& window, float scale);

// Gathers every triangle into framebuffer pixel space (capturing NCS depth first,
// since the viewport map drops z), applies backface culling, and globally
// depth-sorts (painter's algorithm). Must run on the NCS-space buffer, i.e. after
// clipping but BEFORE ViewportFlat. Governed by the AppConfig backface_cull /
// cull_ccw / depth_sort / depth_ascending flags.
void BuildSortedTrianglesFlat(const GeometryBuffer& gb, const std::vector<ObjectSlice>& slices,
                              const Window& window, float scale, std::vector<SortedTri>& out);
