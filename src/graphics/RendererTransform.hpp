#pragma once
#include <vector>
#include "Object.hpp"
#include "GeometryBuffer.hpp"
#include "RenderedObject.hpp" // SortedTri
#include "Mat4.hpp"

// CPU drivers over the shared per-element stages in PipelineStages.hpp: each is a
// cg_parallel_chunks loop calling the same body the CUDA kernels use.

// Flatten the display-file objects into a single SoA GeometryBuffer (the "indexed"
// shape) and fill the ObjectSlice table. `pos` is left in MODEL space here; the
// transform stage bakes the matrices in. When `shading` is on, world/normal arrays
// are allocated so the transform stage can populate them.
void BuildGeometryBuffer(const std::vector<core::Object>& src,
                         GeometryBuffer& gb, std::vector<ObjectSlice>& slices, bool shading);

// Per-vertex map over the pool: world = transform*model (shading), pos = ncs*world.
// When shading, also builds smooth world normals (face-normal scatter + normalize).
void TransformFlat(GeometryBuffer& gb, const std::vector<ObjectSlice>& slices,
                   const core::mat4& ncs_mat, bool shading);

// Per-vertex map: pos = mat * pos (perspective divide inside mat4*Point).
void ProjectFlat(GeometryBuffer& gb, const core::mat4& mat);

// Maps point/line vertices from NCS to framebuffer pixel space (NCSToViewport then
// *scale). cw/ch are the display canvas size; triangle verts are skipped (already
// consumed by BuildSortedTrianglesFlat). Relies on the expanded clip-output layout.
void ViewportFlat(GeometryBuffer& gb, float cw, float ch, float scale);

// Gathers every triangle into framebuffer pixel space (NCS depth captured first),
// backface-culls, and globally depth-sorts (painter's). cw/ch/scale are the canvas
// size + SSAA factor. Must run on the NCS buffer (after clipping, before ViewportFlat).
void BuildSortedTrianglesFlat(const GeometryBuffer& gb, const std::vector<ObjectSlice>& slices,
                              float cw, float ch, float scale, std::vector<SortedTri>& out);
