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
}

inline std::string format(float x, int precision) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << x;
    return stream.str();
}
