#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include "Point.hpp"
#include "Object.hpp"   // core::ObjectType
#include "Shading.hpp"  // ShadeMaterial
#include "Mat4.hpp"
#include "imgui.h"

// Per-object render metadata, indexed by the per-primitive *Obj arrays of a
// GeometryBuffer. This small, scene-sized table replaces the scalars that used to
// live on every RenderedObject (color / filled / material / transform). Keeping it
// out of the per-vertex buffers means the hot pipeline loops touch only floats.
struct ObjectSlice {
    core::ObjectType type   = core::ObjectType::NONE;
    ImU32            color  = 0xFF;   // flat draw color (Material::color)
    bool             filled = false;  // Material::filled (drives the clip rules)
    ShadeMaterial    shadeMat;        // ka/kd/ks/shininess (used when shading on)
    core::mat4       transform;       // model -> world
};

// All scene geometry flattened into a handful of contiguous arrays (SoA). This is
// the CPU stand-in for the device buffers a CUDA port would upload: every pipeline
// stage reads/writes these by index, with no per-object C++ objects on the hot
// path, so the kernels become a near-mechanical translation.
//
//   - Vertices live in one pool: `pos` (always), plus `world`/`normal` when
//     shading is on. 3 floats per vertex, parallel arrays.
//   - Primitives reference the pool by index (pointIdx/lineIdx/triIdx).
//   - `*Obj` give the owning ObjectSlice for each primitive.
//
// Two shapes flow through the pipeline, both using this same layout:
//   - Indexed (fresh from BuildGeometryBuffer): vertices are shared between the
//     primitives of an object; `vobj` is populated so the transform stage can pick
//     a per-object matrix.
//   - Expanded (output of the clip stages): every primitive owns its own vertices
//     laid out as [point verts][line verts][tri verts]; `vobj` is left empty
//     because the remaining stages use global matrices only.
struct GeometryBuffer {
    // vertex pool (3 floats per vertex)
    std::vector<float> pos;     // working-space position (mutated by transform stages)
    std::vector<float> world;   // world-space position (shading; empty otherwise)
    std::vector<float> normal;  // world-space normal   (shading; empty otherwise)

    // per-vertex owning slice — only set on the indexed build buffer (the transform
    // stage needs a per-object matrix). Clip output leaves this empty.
    std::vector<int32_t> vobj;

    // primitives: indices into the vertex pool
    std::vector<uint32_t> pointIdx; // 1 per point
    std::vector<uint32_t> lineIdx;  // 2 per line
    std::vector<uint32_t> triIdx;   // 3 per triangle

    // owning ObjectSlice index per primitive
    std::vector<int32_t> pointObj;
    std::vector<int32_t> lineObj;
    std::vector<int32_t> triObj;

    std::size_t vertexCount() const { return pos.size() / 3; }
    std::size_t pointCount()  const { return pointIdx.size(); }
    std::size_t lineCount()   const { return lineIdx.size() / 2; }
    std::size_t triCount()    const { return triIdx.size() / 3; }
    bool        shaded()      const { return !world.empty(); }

    core::Point getPos(uint32_t v)    const { return { pos[3*v],    pos[3*v+1],    pos[3*v+2] }; }
    core::Point getWorld(uint32_t v)  const { return { world[3*v],  world[3*v+1],  world[3*v+2] }; }
    core::Point getNormal(uint32_t v) const { return { normal[3*v], normal[3*v+1], normal[3*v+2] }; }

    void clear() {
        pos.clear(); world.clear(); normal.clear(); vobj.clear();
        pointIdx.clear(); lineIdx.clear(); triIdx.clear();
        pointObj.clear(); lineObj.clear(); triObj.clear();
    }
};
