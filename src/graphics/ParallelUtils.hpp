#pragma once

// Seleciona o método de paralelismo. O TBB precisa ser instalado no windows e linux,
// para evitar isso, caso o usuário não o tenha instalado, utilizamos uma implementação
// nativa (Que pode ser um pouco pior por não fazer a mesma gestão otimizada do workload).
//
// Usa função template em vez de macro para evitar problemas com vírgulas dentro
// de lambdas que contenha inicializadores de struct ({a, b, c}).

#ifdef USE_TBB_EXECUTION
    #include <execution>
    #include <algorithm>

    template<typename Iterator, typename Function>
    inline void cg_parallel_for_each(Iterator begin, Iterator end, Function func) {
        std::for_each(std::execution::par_unseq, begin, end, func);
    }

#else
    #include <future>
    #include <thread>
    #include <vector>

    template <typename Iterator, typename Function>
    void cg_parallel_for_each(Iterator begin, Iterator end, Function func){
        auto total_elements = std::distance(begin, end);
        if(total_elements == 0) return;
        unsigned int num_threads = std::thread::hardware_concurrency();
        if(num_threads == 0) num_threads = 2;
        if(total_elements < 1000) num_threads = 1;

        auto chunk_size = total_elements / num_threads;
        std::vector<std::future<void>> futures;
        auto chunk_start = begin;
        for(unsigned int i = 0; i < num_threads - 1; i++){
            auto chunk_end = chunk_start;
            std::advance(chunk_end, chunk_size);
            futures.push_back(std::async(std::launch::async, [chunk_start, chunk_end, func](){
                for(auto it = chunk_start; it != chunk_end; ++it){
                    func(*it);
                }
            }));
            chunk_start = chunk_end; // avança para o próximo chunk
        }
        // thread principal processa o último chunk
        for(auto it = chunk_start; it != end; ++it){
            func(*it);
        }
        for(auto &f: futures){
            f.wait();
        }
    }

#endif

// ─── for_each sem o guarda de serialização ───────────────────────────────────
// cg_parallel_for_each serializa entradas com menos de 1000 elementos, o que é
// certo quando cada elemento é leve, mas errado quando cada elemento é um MESH
// INTEIRO: uma cena com 3 objetos de 100k faces cada rodava numa thread só.
// Use esta variante quando o custo por elemento é alto (os estágios do
// renderizador que percorrem malhas). Para uma malha única e gigante, o ganho
// vem do split interno por triângulos/vértices, não daqui.
#ifdef USE_TBB_EXECUTION
    // `par` e não `par_unseq`: os estágios que usam esta função alocam, movem
    // vetores e (nas malhas grandes) pegam um mutex ao juntar os buckets por
    // thread. `unseq` permite que a implementação intercale as invocações no
    // mesmo thread, o que proíbe justamente esse tipo de operação; `par` só
    // exige que os elementos sejam independentes, que é o nosso caso.
    template<typename Iterator, typename Function>
    inline void cg_parallel_for_each_heavy(Iterator begin, Iterator end, Function func) {
        std::for_each(std::execution::par, begin, end, func);
    }
#else
    template <typename Iterator, typename Function>
    void cg_parallel_for_each_heavy(Iterator begin, Iterator end, Function func) {
        auto total = std::distance(begin, end);
        if (total == 0) return;
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 2;
        if ((decltype(total))num_threads > total) num_threads = (unsigned int)total;

        auto chunk = total / num_threads;
        std::vector<std::future<void>> futures;
        auto chunk_start = begin;
        for (unsigned int i = 0; i + 1 < num_threads; ++i) {
            auto chunk_end = chunk_start;
            std::advance(chunk_end, chunk);
            futures.push_back(std::async(std::launch::async, [chunk_start, chunk_end, func]() {
                for (auto it = chunk_start; it != chunk_end; ++it) func(*it);
            }));
            chunk_start = chunk_end;
        }
        for (auto it = chunk_start; it != end; ++it) func(*it); // thread principal
        for (auto& f : futures) f.wait();
    }
#endif

// ─── Paralelismo de blocos grossos (coarse-grained) ──────────────────────────
// Divide o intervalo [0, count) em ~um bloco contíguo por thread de hardware e
// chama func(begin, end) para cada bloco. Diferente de cg_parallel_for_each,
// NÃO há o guarda de serialização para entradas pequenas: use quando cada bloco
// já é pesado (ex.: uma faixa do framebuffer que varre a cena inteira), e não ao
// iterar muitos elementos leves.
#include <thread>
#include <algorithm>
#include <cstddef>

#ifdef USE_TBB_EXECUTION
    // Com TBB, reusamos o pool de threads persistente e o work-stealing dele em
    // vez de criar/destruir threads a cada frame. O simple_partitioner + grão de
    // ~count/nthreads limita a divisão a um bloco por thread (cada bloco varre a
    // cena toda, então não queremos subdividir demais).
    #include <tbb/parallel_for.h>
    #include <tbb/blocked_range.h>

    template <typename Function>
    void cg_parallel_chunks(std::size_t count, Function func) {
        if (count == 0) return;
        unsigned int nt = std::thread::hardware_concurrency();
        if (nt == 0) nt = 4;
        std::size_t grain = (count + nt - 1) / nt;
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, count, grain),
            [&](const tbb::blocked_range<std::size_t>& r) { func(r.begin(), r.end()); },
            tbb::simple_partitioner());
    }

#else
    // Sem TBB: fallback ingênuo com std::async (uma thread por bloco). Pior que o
    // pool do TBB, mas só é usado nos builds sem TBB (ex.: Windows standalone).
    #include <future>
    #include <vector>

    template <typename Function>
    void cg_parallel_chunks(std::size_t count, Function func) {
        if (count == 0) return;
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
        if (num_threads > count) num_threads = (unsigned int)count;

        std::size_t chunk = (count + num_threads - 1) / num_threads;
        std::vector<std::future<void>> futures;
        for (unsigned int t = 1; t < num_threads; ++t) {
            std::size_t begin = (std::size_t)t * chunk;
            std::size_t end = std::min(count, begin + chunk);
            if (begin >= end) break;
            futures.push_back(std::async(std::launch::async, [=]() { func(begin, end); }));
        }
        func(0, std::min(count, chunk)); // thread principal processa o primeiro bloco
        for (auto& f : futures) f.wait();
    }
#endif
