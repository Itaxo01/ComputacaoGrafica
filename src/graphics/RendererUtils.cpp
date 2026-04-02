#include "RendererUtils.hpp"
#include "Window.hpp"
#include <execution>
#include <atomic>

/* Seleção simples verificando os limites da window */
std::vector<core::Point> ClipPoints(const std::vector<core::Point> &v, const core::Point &wp0, const core::Point &wp1){
    std::vector<core::Point> ret(v.size());
    std::atomic<size_t> count{0}; // Utilizado para se livrar do mutex ao inserir no vetor
    
    std::for_each(v.begin(), v.end(), [&](const core::Point &p){
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

std::vector<core::Line> ClipLines(const std::vector<core::Line> &v, const core::Point &wp0, const core::Point &wp1){
    std::vector<core::Line> ret(v.size());

    std::atomic<size_t> count{0}; // Utilizado para se livrar do mutex ao inserir no vetor
    std::for_each(std::execution::par_unseq, v.begin(), v.end(), [&](const core::Line &line) {
        float u1 = 0.0f, u2 = 1.0f;
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
    });
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

    std::for_each(std::execution::par_unseq, v.begin(), v.end(), [&](const core::Wireframe &w) {
        size_t p_count = w.points.size();
        if (p_count < 2) return; 

        // Quebramos o wireframe em segmentos de reta.
        for (size_t i = 0; i < p_count - 1; ++i) {
            core::Line line;
            line.a = w.points[i];
            line.b = w.points[i+1];
            
            // Reutilizamos o Liang-Barsky aqui. Poderia ser chamado ClipLines de novo, porém fazendo aqui permite popularmos o vetor diretamente.
            float u1 = 0.0f, u2 = 1.0f;
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
    });

    ret.resize(count.load(std::memory_order_relaxed));

    return ret;
}



void ViewportTransform(std::vector<core::Point> &v, const Window &window){
    std::for_each(std::execution::par_unseq, v.begin(), v.end(), [&](core::Point &point){
        point = window.WorldToViewport(point);
    });
}

void ViewportTransform(std::vector<core::Line> &v, const Window &window){
    std::for_each(std::execution::par_unseq, v.begin(), v.end(), [&](core::Line &line){
        line.a = window.WorldToViewport(line.a);
        line.b = window.WorldToViewport(line.b);
    });
}