#include "RendererTransform.hpp"
#include "ParallelUtils.hpp"

void TransformToNCS(std::vector<core::Point> &v, const core::mat4 &mat){
    cg_parallel_for_each(v.begin(), v.end(), [&](core::Point &p){ 
        core::Point np = mat * p; 
        p.x = np.x; 
        p.y = np.y; 
        p.z = np.z; 
    });
}

void TransformToNCS(std::vector<core::Line> &v, const core::mat4 &mat){
    cg_parallel_for_each(v.begin(), v.end(), [&](core::Line &l){ l.a = mat * l.a; l.b = mat * l.b; });
}

void TransformToNCS(std::vector<core::Wireframe> &v, const core::mat4 &mat){
    // Em wireframes iteramos pelas linhas internas do mesmo objeto também
    cg_parallel_for_each(v.begin(), v.end(), [&](core::Wireframe &w){ 
        for(auto &p: w.points) p = mat * p; 
    });
}

void TransformToNCS(std::vector<core::Polygon> &v, const core::mat4 &mat){
    cg_parallel_for_each(v.begin(), v.end(), [&](core::Polygon &v){ 
        for(auto &p: v.points) p = mat * p; 
    });
}


void TransformToViewport(std::vector<core::Point> &v, const Window &window, const ImVec2 &offset){
    cg_parallel_for_each(v.begin(), v.end(), [&](core::Point &p){
        core::Point np = window.NCSToViewport(p);
        p.x = np.x + offset.x; 
        p.y = np.y + offset.y;
        p.z = np.z;
    });
}

void TransformToViewport(std::vector<core::Line> &v, const Window &window, const ImVec2 &offset){
    cg_parallel_for_each(v.begin(), v.end(), [&](core::Line &l){
        l.a = window.NCSToViewport(l.a); 
        l.a.x += offset.x; l.a.y += offset.y;
        l.b = window.NCSToViewport(l.b); 
        l.b.x += offset.x; l.b.y += offset.y;
    });
}
void TransformToViewport(std::vector<core::Polygon> &v, const Window &window, const ImVec2 &offset){
    cg_parallel_for_each(v.begin(), v.end(), [&](core::Polygon &p) {
        for(core::Point& pt: p.points){
            pt = window.NCSToViewport(pt);
            pt.x += offset.x, pt.y+=offset.y;
        }
    });
}


