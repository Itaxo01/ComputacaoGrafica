#include "AppConfig.hpp"

namespace AppConfig {
    bool is3d = true;
    bool perspective = true;
    bool render_names = false;
    bool show_bounding_box = false;

    bool backface_cull   = true;
    bool cull_ccw        = true;
    bool depth_sort      = true;
    bool depth_ascending = false;

    int supersample = 2; // SSAA factor: framebuffer is rendered at NxN the viewport
}
