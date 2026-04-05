#include "RendererUtils.hpp"
#include "Window.hpp"
#include <atomic>
#include <utility>

// Seleciona o método de paralelismo. O TBB precisa ser instalado no windows e linux, para evitar isso, caso o usuário não o tenha instalado, utilizamos uma implementação nativa (Que pode ser um pouco pior por não fazer a mesma gestão otimizada do workload).
#ifdef USE_TBB_EXECUTION
    #include <execution>
    #include <algorithm>
    #define PARALLEL_FOR_EACH(begin, end, lambda) \
        std::for_each(std::execution::par_unseq, begin, end, lambda)
#else
    #include <future>
    #include <thread>
    #include <vector>

    template <typename Iterator, typename Function>
    void NativeForEach(Iterator begin, Iterator end, Function func){
        auto total_elements = std::distance(begin, end);
        if(total_elements == 0) return;
        unsigned int num_threads = std::thread::hardware_concurrency(); // Esse valor pode não ser exato/definido em casos específicos.
        if(num_threads == 0) num_threads = 2; // Default fallback
        if(total_elements < 1000) num_threads = 1; // Entrada pequena, fazemos sequencialmente

        auto chunk_size = total_elements / num_threads;
        std::vector<std::future<void>> futures; // futures é um conjunto assincrono para esperar pelas threads terminarem o work. Como o retorno das nossas funções é void, future<void>.
        auto chunk_start = begin;
        for(unsigned int i = 0; i < num_threads - 1; i++){ // não roda para a main thread
            auto chunk_end = chunk_start;
            std::advance(chunk_end, chunk_size);
            futures.push_back(std::async(std::launch::async, [chunk_start, chunk_end, &func](){
                for(auto it = chunk_start; it != chunk_end; ++it){
                    func(*it);
                }
            }));
        }
        // roda para a thread principal
        for(auto it = chunk_start; it != end; ++it){
            func(*it);
        }
        for(auto &f: futures){
            f.wait();
        }
}
    #define PARALLEL_FOR_EACH(begin, end, lambda) \
        NativeForEach(begin, end, lambda)
#endif 

/* Seleção simples verificando os limites da window */
std::vector<core::Point> ClipPoints(const std::vector<core::Point> &v, const core::Point &wp0, const core::Point &wp1){
    std::vector<core::Point> ret(v.size());
    std::atomic<size_t> count{0}; // Utilizado para se livrar do mutex ao inserir no vetor
    
    PARALLEL_FOR_EACH(v.begin(), v.end(), [&](const core::Point &p){
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

std::vector<core::Line> ClipLines(const std::vector<core::Line> &v, const core::Point &wp0, const core::Point &wp1){
    std::vector<core::Line> ret(v.size());

    std::atomic<size_t> count{0}; // Utilizado para se livrar do mutex ao inserir no vetor
    PARALLEL_FOR_EACH(v.begin(), v.end(), [&](const core::Line &line) {
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

    PARALLEL_FOR_EACH(v.begin(), v.end(), [&](const core::Wireframe &w) {
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

                size_t insert_index = count.fetch_add(1, std::memory_order_relaxed);
                ret[insert_index] = cLine;
            }
        }
    });

    ret.resize(count.load(std::memory_order_relaxed));

    return ret;
}

void TransformToNCS(std::vector<core::Point> &v, const core::Matrix<float> &mat){
    PARALLEL_FOR_EACH(v.begin(), v.end(), [&](core::Point &p){ p = mat * p; });
}

void TransformToNCS(std::vector<core::Line> &v, const core::Matrix<float> &mat){
    PARALLEL_FOR_EACH(v.begin(), v.end(), [&](core::Line &l){ l.a = mat * l.a; l.b = mat * l.b; });
}

void TransformToNCS(std::vector<core::Wireframe> &v, const core::Matrix<float> &mat){
    // Em wireframes iteramos pelas linhas internas do mesmo objeto também
    PARALLEL_FOR_EACH(v.begin(), v.end(), [&](core::Wireframe &w){ 
        for(auto &p: w.points) p = mat * p; 
    });
}

void TransformToViewport(std::vector<core::Point> &v, const Window &window, const ImVec2 &offset){
    PARALLEL_FOR_EACH(v.begin(), v.end(), [&](core::Point &p){
        p = window.NCSToViewport(p);
        p.x += offset.x; 
        p.y += offset.y;
    });
}

void TransformToViewport(std::vector<core::Line> &v, const Window &window, const ImVec2 &offset){
    PARALLEL_FOR_EACH(v.begin(), v.end(), [&](core::Line &l){
        l.a = window.NCSToViewport(l.a); 
        l.a.x += offset.x; l.a.y += offset.y;
        l.b = window.NCSToViewport(l.b); 
        l.b.x += offset.x; l.b.y += offset.y;
    });
}
