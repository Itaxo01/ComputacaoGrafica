#pragma once
#include "CudaCommon.cuh"
#include "Mat4.hpp"

// Host-callable launchers for the geometry stages. Each wraps a kernel that is a
// one-line call into the shared PipelineStages.hpp functions.
namespace cuda {

void launchTransform(DGeo& g, const ObjectSlice* dslice, const core::mat4& ncs, bool shaded);
void launchProject(DGeo& g, const core::mat4& mat);

// Bounded worst-case pre-size + atomic-append clip, then shrink `out` to the realized
// counts. `counters` is a scratch DBuf<unsigned>(4) reused across calls.
void launchClip(const DGeo& in, DGeo& out, bool isNear, const ObjectSlice* dslice,
                const ClipBox& box, float nearZ, int mode, DBuf<unsigned>& counters);

// Build viewport-space SortedTri into `outSorted`; returns the count. `counter` is a
// scratch DBuf<unsigned>(1). Sorts by depth when doSort is set.
size_t launchBuildSorted(DGeo& g, const ObjectSlice* dslice, DBuf<SortedTri>& outSorted,
                         DBuf<unsigned>& counter, float cw, float ch, float scale,
                         bool cull, bool cullCcw, bool doSort, bool ascending);

} // namespace cuda
