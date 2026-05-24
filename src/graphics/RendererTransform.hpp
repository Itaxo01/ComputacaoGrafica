#pragma once
#include <vector>
#include "Object.hpp"
#include "Window.hpp"
#include "Mat4.hpp"
#include "imgui.h"

void TransformToNCS(std::vector<core::Object>& objs, const core::mat4& ncs_mat);
void TransformToViewport(std::vector<core::Object>& objs, const Window& window, const ImVec2& offset);
