#include "AppConfig.hpp"

namespace AppConfig {
    bool is3d = true;
    bool perspective = true;
    bool render_names = false;
    bool show_bounding_box = false;

    bool show_axes = false;
    bool show_grid = false;
    bool show_axis_coordinates = false;
    int  clipping_mode = 0; // 0 = Liang-Barsky, 1 = Cohen-Sutherland

    bool backface_cull   = true;
    bool cull_ccw        = true;
    bool depth_sort      = true;
    bool depth_ascending = false;

    int supersample = 2; // SSAA factor: framebuffer is rendered at NxN the viewport

    bool z_buffer = true; // per-pixel depth test; when off, falls back to painter's sort

#ifdef USE_TBB_EXECUTION
    const bool tbb_available = true;
#else
    const bool tbb_available = false;
#endif
    bool use_tbb = tbb_available; // locked to false when the build has no TBB
}
