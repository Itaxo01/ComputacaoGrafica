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

    // Solid-mesh rendering (imported OBJ meshes). Toggleable at runtime because the
    // correct winding/depth sign depends on the model and can't be known up front.
    extern bool backface_cull;   // skip triangles facing away from the camera
    extern bool cull_ccw;        // winding convention for the cull test (flip if the wrong side is culled)
    extern bool depth_sort;      // painter's algorithm: sort triangles back-to-front
    extern bool depth_ascending; // flip sort order (use if near/far appear inverted)
}

inline std::string format(float x, int precision) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << x;
    return stream.str();
}
