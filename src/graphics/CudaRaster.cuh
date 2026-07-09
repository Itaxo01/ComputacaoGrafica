#pragma once
#include "CudaCommon.cuh"
#include "Light.hpp"
#include "Material.hpp" // core::Color3

namespace cuda {

// Rasterize the device-resident geometry into fb (clear → triangles (packed atomicMin
// z-buffer) → split → lines/points → premultiplied SSAA resolve). Fills fb.resolved
// (device); the caller copies it to the host. The triangle fill reuses the shared
// rasterizeTriangleInto(); only the pixel commit (atomicMin) is GPU-specific.
void rasterScene(DeviceFramebuffer& fb, const SortedTri* sorted, size_t sortedCount,
                 DGeo& gRender, const ObjectSlice* dslice,
                 int mode, core::Point eye, core::Color3 ambient,
                 const core::Light* lights, int nLights,
                 bool zbuffer, bool depthLess, float cw, float ch, float scale, int supersample);

} // namespace cuda
