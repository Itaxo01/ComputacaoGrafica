#pragma once
#include <vector>
#include "Object.hpp"
#include "RenderedObject.hpp"
#include "Window.hpp"
#include "Mat4.hpp"
#include "imgui.h"

// Converts display-file objects into RenderedObjects, baking obj.transform and
// ncs_mat into vertex positions in a single pass (avoids a separate transform loop).
void TransformObjectAndDoNCS(std::vector<RenderedObject>& dest,
                              const std::vector<core::Object>& src,
                              const core::mat4& ncs_mat);

// Multiplies every vertex by `mat` (the divide-by-w happens inside mat4 * Point).
// Used by the perspective path to project VRC-space vertices to NCS after the
// near-plane clip has run.
void ProjectVertices(std::vector<RenderedObject>& objs, const core::mat4& mat);

// Maps every vertex from NCS to framebuffer pixel space: NCSToViewport gives
// viewport-local pixels, which are then multiplied by `scale` (the framebuffer
// supersample factor). No screen offset is added — the framebuffer is positioned
// on screen by the AddImage blit, not by per-vertex offsets.
void TransformToViewport(std::vector<RenderedObject>& objs, const Window& window, float scale);

// Gathers every filled triangle into framebuffer pixel space (capturing NCS-space
// depth first, since the viewport map drops z), applies backface culling, and
// globally depth-sorts them (painter's algorithm). Coordinates are scaled by
// `scale` like TransformToViewport. Must run on NCS-space `objs`, i.e. after
// clipping but BEFORE TransformToViewport. Behavior is governed by the AppConfig
// backface_cull / cull_ccw / depth_sort / depth_ascending flags.
void BuildSortedTriangles(const std::vector<RenderedObject>& objs, const Window& window,
                          float scale, std::vector<SortedTri>& out);
