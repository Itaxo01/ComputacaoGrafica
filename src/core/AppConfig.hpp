#pragma once
#include <iomanip>
#include <sstream>
#include <string>

namespace AppConfig {
    extern bool is3d;
    extern bool perspective;
    extern bool render_names;
    extern bool use_object_color;
    extern bool show_bounding_box;

    // Viewport background helpers (axes/grid overlay) and the clipping algorithm.
    extern bool show_axes;
    extern bool show_grid;
    extern bool show_axis_coordinates;
    extern int  clipping_mode; // 0 = Liang-Barsky, 1 = Cohen-Sutherland

    // Solid-mesh rendering (imported OBJ meshes). Toggleable at runtime because the
    // correct winding/depth sign depends on the model and can't be known up front.
    extern bool backface_cull;   // skip triangles facing away from the camera
    extern bool cull_ccw;        // winding convention for the cull test (flip if the wrong side is culled)
    extern bool depth_sort;      // painter's algorithm: sort triangles back-to-front
    extern bool depth_ascending; // flip sort order (use if near/far appear inverted)

    // Supersampling anti-aliasing factor. The CPU framebuffer is rasterized at
    // (supersample x viewport) resolution and box-downsampled on present. 1 = off.
    extern int supersample;

    // Per-pixel depth buffer (z-buffer). When on, visibility is resolved per pixel
    // and the painter's triangle sort is skipped. When off, uses depth_sort.
    // The test direction follows depth_ascending (flip if a model looks inside-out).
    extern bool z_buffer;
}

inline std::string format(float x, int precision) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << x;
    return stream.str();
}
