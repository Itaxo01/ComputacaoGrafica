#include "RendererUtils.hpp"
#include "Line.hpp"
#include "ParallelUtils.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "Window.hpp"
#include <atomic>
#include <utility>

/* Seleção simples verificando os limites da window */
std::vector<core::Point> ClipPoints(const std::vector<core::Point> &v, const core::Point &wp0, const core::Point &wp1){
    std::vector<core::Point> ret(v.size());
    std::atomic<size_t> count{0}; // Utilizado para se livrar do mutex ao inserir no vetor
    
    cg_parallel_for_each(v.begin(), v.end(), [&](const core::Point &p){
        if(p.x >= wp0.x && p.x <= wp1.x && p.y >= wp0.y && p.y <= wp1.y){
            size_t insert_index = count.fetch_add(1, std::memory_order_relaxed);
            ret[insert_index] = p;
        }
    });
    ret.resize(count.load(std::memory_order_relaxed));
    return ret;
}

inline bool LineClipTest(float p, float q, float &u1, float &u2){
    if(p == 0.0f){ // A linha é paralela a algum limite da window
        if(q < 0.0) return false; // paralela e completamente fora da window.
    } else {
        float r = q/p;
        if (p < 0.0f) {
            if (r > u2) return false;
            if (r > u1) u1 = r;
        } else {
            if (r < u1) return false;
            if (r < u2) u2 = r;
        }
    }
    return true;
}

/* Usada para alguma coisa que não é renderização, acho que o background talvez*/
std::pair<core::Line, bool> ClipLine(const core::Line &line, const core::Point &wp0, const core::Point&wp1) {
    float u1 = 0.0f;
    float u2 = 1.0f;
    float dx = line.b.x - line.a.x;
    float dy = line.b.y - line.a.y;
    if(LineClipTest(-dx, line.a.x - wp0.x, u1, u2) && // left
        LineClipTest(dx, wp1.x - line.a.x, u1, u2)  && // right
        LineClipTest(-dy, line.a.y - wp0.y, u1, u2)  && // bottom
        LineClipTest(dy, wp1.y - line.a.y, u1, u2)     // top
    ){
        core::Line cLine = line;
        if(u1 > 0.0f){
            cLine.a.x = line.a.x + u1 * dx;
            cLine.a.y = line.a.y + u1 * dy;
        }
        if(u2 < 1.0f){
            cLine.b.x = line.a.x + u2 * dx;
            cLine.b.y = line.a.y + u2 * dy;
        }
        return std::make_pair(cLine, true);
    }
    return std::make_pair(line, false);
}

static inline void ClipLineLiangBarsky(const core::Line &line, const core::Point &wp0, const core::Point &wp1, std::vector<core::Line> &ret, std::atomic<size_t> &count) {
    float u1 = 0.0f;
    float u2 = 1.0f;
    float dx = line.b.x - line.a.x;
    float dy = line.b.y - line.a.y;
    if(LineClipTest(-dx, line.a.x - wp0.x, u1, u2) && // left
        LineClipTest(dx, wp1.x - line.a.x, u1, u2)  && // right
        LineClipTest(-dy, line.a.y - wp0.y, u1, u2)  && // bottom
        LineClipTest(dy, wp1.y - line.a.y, u1, u2)     // top
    ){
        core::Line cLine = line;
        if(u1 > 0.0f){
            cLine.a.x = line.a.x + u1 * dx;
            cLine.a.y = line.a.y + u1 * dy;
        }
        if(u2 < 1.0f){
            cLine.b.x = line.a.x + u2 * dx;
            cLine.b.y = line.a.y + u2 * dy;
        }
        size_t insert_index = count.fetch_add(1, std::memory_order_relaxed);
        ret[insert_index] = cLine;
    }
}

static inline void ClipLineCohenSutherland(const core::Line &line, const core::Point &wp0, const core::Point &wp1, std::vector<core::Line> &ret, std::atomic<size_t> &count) {
    // TODO
}

std::vector<core::Line> ClipLines(const std::vector<core::Line> &v, const core::Point &wp0, const core::Point &wp1, int mode){
    std::vector<core::Line> ret(v.size());
    std::atomic<size_t> count{0};

    if (mode == 0) {
        cg_parallel_for_each(v.begin(), v.end(), [&](const core::Line &l){ ClipLineLiangBarsky(l, wp0, wp1, ret, count); });
    } else {
        cg_parallel_for_each(v.begin(), v.end(), [&](const core::Line &l){ ClipLineCohenSutherland(l, wp0, wp1, ret, count); });
    }

    ret.resize(count.load(std::memory_order_relaxed));
    return ret;
}


/* */
std::vector<core::Line> ClipWireframes(const std::vector<core::Wireframe> &v, const core::Point &wp0, const core::Point &wp1){
    size_t max_lines = 0;
    for(const auto &w: v){
        if(w.points.size() > 1) {
            max_lines += w.points.size() - 1; // n pontos = n-1 linhas.
        }
    }

    std::vector<core::Line> ret(max_lines);
    std::atomic<size_t> count{0};

    cg_parallel_for_each(v.begin(), v.end(), [&](const core::Wireframe &w) {
        size_t p_count = w.points.size();
        if (p_count < 2) return; 

        // Quebramos o wireframe em segmentos de reta.
        for (size_t i = 0; i < p_count - 1; ++i) {
            core::Line line;
            line.a = w.points[i];
            line.b = w.points[i+1];
            
            // Reutilizamos o Liang-Barsky aqui. Poderia ser chamado ClipLines de novo, porém fazendo aqui permite popularmos o vetor diretamente.
            float u1 = 0.0f;
            float u2 = 1.0f;
            float dx = line.b.x - line.a.x;
            float dy = line.b.y - line.a.y;
            
            if(LineClipTest(-dx, line.a.x - wp0.x, u1, u2) && // left
               LineClipTest(dx, wp1.x - line.a.x, u1, u2)  && // right
               LineClipTest(-dy, line.a.y - wp0.y, u1, u2)  && // bottom
               LineClipTest(dy, wp1.y - line.a.y, u1, u2)     // top
            ){
                core::Line cLine = line;
                if(u1 > 0.0f){
                    cLine.a.x = line.a.x + u1 * dx;
                    cLine.a.y = line.a.y + u1 * dy;
                }
                if(u2 < 1.0f){
                    cLine.b.x = line.a.x + u2 * dx;
                    cLine.b.y = line.a.y + u2 * dy;
                }
                #ifndef DONT_USE_OBJECT_COLOR
                    cLine.object_color = w.object_color;
                #endif

                size_t insert_index = count.fetch_add(1, std::memory_order_relaxed);
                ret[insert_index] = cLine;
            }
        }
    });

    ret.resize(count.load(std::memory_order_relaxed));

    return ret;
}

std::vector<core::Polygon> ClipPolygon(const std::vector<core::Polygon> &v, const core::Point &wp0, const core::Point &wp1){
    // TODO
}


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


