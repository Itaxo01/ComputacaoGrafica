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
