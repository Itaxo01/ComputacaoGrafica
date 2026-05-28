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

void TransformToViewport(std::vector<RenderedObject>& objs, const Window& window, const ImVec2& offset);
