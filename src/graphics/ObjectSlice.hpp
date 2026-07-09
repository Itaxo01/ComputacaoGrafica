#pragma once
#include "Object.hpp"   // core::ObjectType
#include "Shading.hpp"  // ShadeMaterial
#include "Mat4.hpp"
#include "imgui.h"

// Per-object render metadata, indexed by the per-primitive *Obj arrays of a
// GeometryBuffer / GBView. POD (enum / ImU32 / bool / ShadeMaterial / mat4), so the
// same array is uploaded verbatim to the device and used by the shared pipeline
// stages on both CPU and GPU.
struct ObjectSlice {
    core::ObjectType type   = core::ObjectType::NONE;
    ImU32            color  = 0xFF;   // flat draw color (Material::color)
    bool             filled = false;  // Material::filled (drives the clip rules)
    ShadeMaterial    shadeMat;        // ka/kd/ks/shininess (used when shading on)
    core::mat4       transform;       // model -> world
};
