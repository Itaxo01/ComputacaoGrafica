#ifndef RENDERER_CACHE
#define RENDERER_CACHE
#include "Point.hpp"
#include "Window.hpp"
#include "imgui.h"

class RendererCache {
    WindowAttributes l_w;
    ImVec2 l_canvas_p0;
    ImVec2 l_canvas_p1;

public:
    RendererCache(const WindowAttributes& l_w) : l_w(l_w) {}
    RendererCache() {}

    inline bool cache_changed(const WindowAttributes& w,
                               const ImVec2& canvas_p0, const ImVec2& canvas_p1) const {
        return (l_w != w
            || l_canvas_p0.x != canvas_p0.x || l_canvas_p0.y != canvas_p0.y
            || l_canvas_p1.x != canvas_p1.x || l_canvas_p1.y != canvas_p1.y);
    }
    inline void store_cache(const WindowAttributes& w,
                             const ImVec2& canvas_p0, const ImVec2& canvas_p1) {
        l_w = w;
        l_canvas_p0 = canvas_p0;
        l_canvas_p1 = canvas_p1;
    }
};

#endif // RENDERER_CACHE
