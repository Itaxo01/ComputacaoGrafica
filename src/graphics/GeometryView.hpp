#pragma once
#include <cstdint>
#include <cstddef>
#include "Point.hpp"
#include "HostDevice.hpp"

// Non-owning, raw-pointer view over a flat geometry buffer. The pointers can come
// from a host GeometryBuffer (std::vector::data()) or from device allocations
// (cudaMalloc), so the shared per-element pipeline functions in PipelineStages.hpp
// operate through this view and never touch a std::vector — letting the exact same
// code run in a CPU loop and in a CUDA kernel.
//
// Layout matches GeometryBuffer: a vertex pool (pos, plus world/normal when shading)
// indexed by the primitive arrays; *Obj give the owning ObjectSlice per primitive.
struct GBView {
    float*   pos    = nullptr;   // 3 floats/vertex (working space)
    float*   world  = nullptr;   // 3 floats/vertex (shading; null otherwise)
    float*   normal = nullptr;   // 3 floats/vertex (shading; null otherwise)
    int32_t* vobj   = nullptr;   // owning slice per vertex (indexed/build buffer only)

    uint32_t* pointIdx = nullptr;
    uint32_t* lineIdx  = nullptr;
    uint32_t* triIdx   = nullptr;
    int32_t*  pointObj = nullptr;
    int32_t*  lineObj  = nullptr;
    int32_t*  triObj   = nullptr;

    size_t vertexCount = 0, pointCount = 0, lineCount = 0, triCount = 0;
    bool   shaded = false;

    CG_HD core::Point getPos   (uint32_t v) const { return core::Point(pos[3*v],    pos[3*v+1],    pos[3*v+2]); }
    CG_HD core::Point getWorld (uint32_t v) const { return core::Point(world[3*v],  world[3*v+1],  world[3*v+2]); }
    CG_HD core::Point getNormal(uint32_t v) const { return core::Point(normal[3*v], normal[3*v+1], normal[3*v+2]); }
    // const: these mutate the pointed-to buffers, not the view's own pointers.
    CG_HD void setPos   (uint32_t v, const core::Point& p) const { pos[3*v]=p.x;    pos[3*v+1]=p.y;    pos[3*v+2]=p.z; }
    CG_HD void setWorld (uint32_t v, const core::Point& p) const { world[3*v]=p.x;  world[3*v+1]=p.y;  world[3*v+2]=p.z; }
    CG_HD void setNormal(uint32_t v, const core::Point& p) const { normal[3*v]=p.x; normal[3*v+1]=p.y; normal[3*v+2]=p.z; }
};
