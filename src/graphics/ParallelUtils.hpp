#pragma once

// Seleciona o método de paralelismo. Existem duas implementações de cada
// primitiva: a do TBB e uma nativa (std::thread / std::async), usada quando o TBB
// não está instalado.
//
// A escolha é feita em DOIS níveis:
//
//   1. Compilação — `USE_TBB_EXECUTION` (definido pelo Makefile quando o link com
//      -ltbb funciona) decide se o caminho do TBB sequer existe no binário. Sem
//      ele, só a implementação nativa é compilada.
//   2. Execução — `AppConfig::use_tbb` escolhe qual das duas roda, e pode ser
//      trocado pelo checkbox do viewport a qualquer momento. Quando o binário foi
//      compilado sem TBB, `AppConfig::tbb_available` é false e o toggle fica
//      travado em nativo.
//
// O despacho custa UM branch por região paralela (não por elemento): são algumas
// dezenas por frame, contra milhões de iterações dentro delas.
//
// ATENÇÃO ao que o toggle NÃO faz: o binário continua linkado com -ltbb e a
// inicialização estática da biblioteca continua acontecendo. Ele isola diferenças
// de ESCALONAMENTO, não a presença do TBB no processo — para isso é preciso um
// build sem a flag (`make windows`).
//
// Usa função template em vez de macro para evitar problemas com vírgulas dentro
// de lambdas que contenha inicializadores de struct ({a, b, c}).

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <future>
#include <iterator>
#include <thread>
#include <type_traits>
#include <vector>

#include "AppConfig.hpp"

#ifdef USE_TBB_EXECUTION
    #include <execution>
    #include <tbb/parallel_for.h>
    #include <tbb/blocked_range.h>
#endif

// Número de blocos POR THREAD usado por cg_parallel_chunks_balanced. Ver a
// justificativa e as medições logo acima da função.
constexpr unsigned kDefaultChunksPerThread = 8;

namespace cg_par {

// ─── Guarda de aninhamento (só do lado nativo) ───────────────────────────────
// As funções do renderizador se aninham: cg_parallel_for_each_heavy percorre os
// OBJETOS e, dentro de cada um, for_vertices/GatherTriangles chamam
// cg_parallel_chunks sobre os vértices/triângulos daquele objeto.
//
// Para o TBB isso é normal: os dois níveis viram tarefas do MESMO pool, que tem
// um número fixo de threads.
//
// Para o caminho nativo é um desastre: cada nível cria as próprias threads. Com
// 16 threads de hardware, 16 tarefas externas abrindo 16 internas cada dá 256
// threads criadas e destruídas por frame. Este contador marca a thread como "já
// sou worker de uma região nativa"; quando ela entra em outra região, essa região
// roda em série. É thread_local, então cada worker carrega o próprio estado.
//
// O contador só é incrementado quando a região REALMENTE se dividiu: uma região
// que rodou em série na thread chamadora não ocupa ninguém, e o nível de dentro
// continua livre para paralelizar. É o que salva o caso da malha única gigante:
// com um objeto só, o nível de fora nem chega a se dividir, e todo o paralelismo
// útil (que está no nível de dentro, sobre os vértices/triângulos) continua
// valendo — verificado com models/simplify_Donut.obj, 1 objeto de 64k triângulos.
inline thread_local int native_depth = 0;

struct NativeRegion {
    NativeRegion() { ++native_depth; }
    ~NativeRegion() { --native_depth; }
    NativeRegion(const NativeRegion&) = delete;
    NativeRegion& operator=(const NativeRegion&) = delete;
};

inline bool inside_native_region() { return native_depth > 0; }

// ─── Implementação nativa ────────────────────────────────────────────────────
namespace native {

    // for_each sem o guarda de serialização para entradas pequenas: use quando o
    // custo POR ELEMENTO é alto (cada elemento é uma malha inteira).
    //
    // ESCALONAMENTO DINÂMICO, um elemento por vez, e não blocos contíguos fixos.
    // O motivo é o mesmo do chunks_balanced: o custo por elemento aqui é
    // DESCONHECIDO e muito desigual — na cena do donut, 2 dos 777 objetos
    // carregam quase metade dos triângulos. Dividindo em 16 blocos contíguos, o
    // bloco que calhasse de conter a malha pesada definia sozinho o tempo do
    // estágio, sem nada para redistribuir. Um fetch_add por objeto custa ~780
    // atômicas por estágio, contra milhões de vértices processados dentro deles.
    //
    // Medido (donut, 60 frames --sweep, backend nativo, frame inteiro):
    //   blocos estáticos + aninhamento livre   59,1 ms  (e até 256 threads/frame)
    //   blocos estáticos + guarda              57,6 ms
    //   dinâmico + guarda (este)               56,4 ms
    //
    // O ganho é modesto porque o que sobra NÃO é desbalanceamento: é Amdahl. Com
    // a guarda ligada, a malha mais pesada (~46% dos triângulos da cena) é
    // processada inteira por uma thread só, e nenhum reescalonamento do nível de
    // fora contorna isso — só dividir DENTRO dela, que é exatamente o que a
    // guarda proíbe e o que o TBB faz de graça (as duas tarefas moram no mesmo
    // pool). É a razão de o backend nativo ficar em 56 ms onde o TBB faz 35 ms
    // nessa cena, e replicar work stealing aqui não vale para um caminho que só
    // existe como fallback.
    template <typename Iterator, typename Function>
    void for_each_heavy(Iterator begin, Iterator end, Function func) {
        auto total = std::distance(begin, end);
        if (total == 0) return;
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 2;
        if ((decltype(total))num_threads > total) num_threads = (unsigned int)total;

        if (num_threads <= 1 || inside_native_region()) {
            for (auto it = begin; it != end; ++it) func(*it);
            return;
        }

        // O laço dinâmico precisa saltar direto para o i-ésimo elemento; para um
        // iterador que não seja random access, cai no percurso sequencial acima.
        if constexpr (!std::is_base_of_v<std::random_access_iterator_tag,
                          typename std::iterator_traits<Iterator>::iterator_category>) {
            for (auto it = begin; it != end; ++it) func(*it);
        } else {
            const std::size_t n = (std::size_t)total;
            std::atomic<std::size_t> next{0};
            auto worker = [&]() {
                NativeRegion guard;   // esta thread passa a ser worker
                for (;;) {
                    const std::size_t i = next.fetch_add(1, std::memory_order_relaxed);
                    if (i >= n) break;
                    func(*(begin + (typename std::iterator_traits<Iterator>::difference_type)i));
                }
            };

            std::vector<std::thread> pool;
            pool.reserve(num_threads - 1);
            for (unsigned int t = 1; t < num_threads; ++t) pool.emplace_back(worker);
            worker();                 // a thread principal também consome elementos
            for (auto& th : pool) th.join();
        }
    }

    // Blocos grossos: um bloco contíguo por thread de hardware, func(begin, end).
    template <typename Function>
    void chunks(std::size_t count, Function func) {
        if (count == 0) return;
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
        if (num_threads > count) num_threads = (unsigned int)count;

        if (num_threads <= 1 || inside_native_region()) { func(0, count); return; }

        std::size_t chunk = (count + num_threads - 1) / num_threads;
        std::vector<std::future<void>> futures;
        futures.reserve(num_threads - 1);
        for (unsigned int t = 1; t < num_threads; ++t) {
            std::size_t begin = (std::size_t)t * chunk;
            std::size_t end = std::min(count, begin + chunk);
            if (begin >= end) break;
            futures.push_back(std::async(std::launch::async, [=]() {
                NativeRegion guard;
                func(begin, end);
            }));
        }
        {
            NativeRegion guard;       // thread principal processa o primeiro bloco
            func(0, std::min(count, chunk));
        }
        for (auto& f : futures) f.wait();
    }

    // Blocos grossos com balanceamento dinâmico.
    //
    // Sem TBB não existe escalonador para dar blocos: o balanceamento é nosso.
    //
    // Usamos self-scheduling dinâmico (um contador atômico compartilhado) em vez
    // de work stealing com deques por thread. Roubo por deque é a ferramenta certa
    // quando as tarefas geram subtarefas (paralelismo aninhado) e quando se quer
    // execução LIFO local por localidade de cache. Aqui os blocos são um array
    // plano, conhecido de antemão e sem aninhamento: um fetch_add por bloco dá o
    // mesmo balanceamento sem deques, sem escolha aleatória de vítima e sem os
    // problemas de pop concorrente. São ~128 operações atômicas por frame.
    //
    // Também corrige um problema do `chunks` nativo: ele cria uma thread por
    // bloco. Com K=8 isso seriam 128 threads por chamada. Aqui o número de threads
    // continua sendo o de hardware, independentemente do número de blocos.
    template <typename Function>
    void chunks_balanced(std::size_t count, Function func,
                         unsigned chunks_per_thread, std::size_t min_chunk) {
        if (count == 0) return;
        unsigned int nt = std::thread::hardware_concurrency();
        if (nt == 0) nt = 4;
        if (chunks_per_thread < 1) chunks_per_thread = 1;
        if (min_chunk < 1) min_chunk = 1;

        const std::size_t target = (std::size_t)nt * chunks_per_thread;
        std::size_t grain = (count + target - 1) / target;
        if (grain < min_chunk) grain = min_chunk;
        if (grain == 0) grain = 1;

        const std::size_t nchunks = (count + grain - 1) / grain;
        if (nchunks == 1 || nt == 1 || inside_native_region()) { func(0, count); return; }

        std::atomic<std::size_t> next{0};
        auto worker = [&]() {
            NativeRegion guard;
            for (;;) {
                const std::size_t i = next.fetch_add(1, std::memory_order_relaxed);
                if (i >= nchunks) break;
                const std::size_t lo = i * grain;
                const std::size_t hi = std::min(count, lo + grain);
                func(lo, hi);
            }
        };

        unsigned int nthreads = (unsigned int)std::min<std::size_t>(nt, nchunks);
        std::vector<std::thread> pool;
        pool.reserve(nthreads - 1);
        for (unsigned int t = 1; t < nthreads; ++t) pool.emplace_back(worker);
        worker();                       // a thread principal também consome blocos
        for (auto& th : pool) th.join();
    }

} // namespace native

// ─── Implementação TBB ───────────────────────────────────────────────────────
#ifdef USE_TBB_EXECUTION
namespace tbb_impl {

    // `par` e não `par_unseq`: os estágios que usam esta função alocam, movem
    // vetores e (nas malhas grandes) pegam um mutex ao juntar os buckets por
    // thread. `unseq` permite que a implementação intercale as invocações no
    // mesmo thread, o que proíbe justamente esse tipo de operação; `par` só
    // exige que os elementos sejam independentes, que é o nosso caso.
    template <typename Iterator, typename Function>
    inline void for_each_heavy(Iterator begin, Iterator end, Function func) {
        std::for_each(std::execution::par, begin, end, func);
    }

    // Reusamos o pool de threads persistente e o work-stealing do TBB em vez de
    // criar/destruir threads a cada frame. O simple_partitioner + grão de
    // ~count/nthreads limita a divisão a um bloco por thread (cada bloco varre a
    // cena toda, então não queremos subdividir demais).
    template <typename Function>
    void chunks(std::size_t count, Function func) {
        if (count == 0) return;
        unsigned int nt = std::thread::hardware_concurrency();
        if (nt == 0) nt = 4;
        std::size_t grain = (count + nt - 1) / nt;
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, count, grain),
            [&](const tbb::blocked_range<std::size_t>& r) { func(r.begin(), r.end()); },
            tbb::simple_partitioner());
    }

    // simple_partitioner com um grão menor: a bisseção recursiva do TBB desce até
    // blocos de ~grain, gerando nt*K tarefas roubáveis em vez de nt. Continua
    // sendo simple_partitioner (e não auto_partitioner) porque o auto só subdivide
    // DEPOIS que um roubo acontece — reativo, e cada subdivisão extra paga a
    // varredura redundante. Medido: auto com grão 1 = 33,3 ms, auto com piso de
    // grão = 15,5 ms, simple com grão count/(nt*8) = 13,7 ms.
    template <typename Function>
    void chunks_balanced(std::size_t count, Function func,
                         unsigned chunks_per_thread, std::size_t min_chunk) {
        if (count == 0) return;
        unsigned int nt = std::thread::hardware_concurrency();
        if (nt == 0) nt = 4;
        if (chunks_per_thread < 1) chunks_per_thread = 1;
        if (min_chunk < 1) min_chunk = 1;

        const std::size_t target = (std::size_t)nt * chunks_per_thread;
        std::size_t grain = (count + target - 1) / target;
        if (grain < min_chunk) grain = min_chunk;

        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, count, grain),
            [&](const tbb::blocked_range<std::size_t>& r) { func(r.begin(), r.end()); },
            tbb::simple_partitioner());
    }

} // namespace tbb_impl
#endif

} // namespace cg_par

// ─── Despacho ────────────────────────────────────────────────────────────────
// O flag é lido pela thread CHAMADORA, antes da região abrir — os workers nunca o
// consultam. Trocar o toggle no meio de um frame portanto não parte uma região ao
// meio: no máximo o próximo estágio roda no outro backend. Na prática nem isso
// acontece, porque a GUI e o renderizador rodam na mesma thread.

// ─── for_each sem o guarda de serialização ───────────────────────────────────
// Use esta variante quando o custo por elemento é alto (os estágios do
// renderizador que percorrem malhas). Para uma malha única e gigante, o ganho
// vem do split interno por triângulos/vértices, não daqui.
template <typename Iterator, typename Function>
inline void cg_parallel_for_each_heavy(Iterator begin, Iterator end, Function func) {
#ifdef USE_TBB_EXECUTION
    if (AppConfig::use_tbb) { cg_par::tbb_impl::for_each_heavy(begin, end, func); return; }
#endif
    cg_par::native::for_each_heavy(begin, end, func);
}

// ─── Paralelismo de blocos grossos (coarse-grained) ──────────────────────────
// Divide o intervalo [0, count) em ~um bloco contíguo por thread de hardware e
// chama func(begin, end) para cada bloco. NÃO há guarda de serialização para
// entradas pequenas: use quando cada bloco já é pesado (ex.: uma faixa do
// framebuffer que varre a cena inteira), e não ao iterar muitos elementos leves.
template <typename Function>
inline void cg_parallel_chunks(std::size_t count, Function func) {
#ifdef USE_TBB_EXECUTION
    if (AppConfig::use_tbb) { cg_par::tbb_impl::chunks(count, func); return; }
#endif
    cg_par::native::chunks(count, func);
}

// ─── Blocos grossos COM BALANCEAMENTO DINÂMICO ───────────────────────────────
// Mesma interface de cg_parallel_chunks — func(begin, end) sobre blocos contíguos
// de [0, count) — mas divide em K blocos POR THREAD em vez de exatamente um.
//
// POR QUE: cg_parallel_chunks produz um bloco por thread de hardware. Quando o
// custo de cada bloco é UNIFORME isso é ótimo (nada a rebalancear, overhead
// mínimo). Quando o custo depende dos dados, é o pior caso possível: com um
// bloco por thread não sobra nada para redistribuir, e o tempo total passa a ser
// o do bloco mais caro. Foi o que aconteceu no rasterizador — o modelo ocupa uma
// faixa horizontal da tela, 10 das 16 faixas ficavam sem triângulo algum e uma
// única faixa carregava metade do frame (medido: mais carregada/média = 8,25x).
//
// O TBB já faz work stealing (deques por thread, LIFO local, roubo FIFO da outra
// ponta). O problema nunca foi o escalonador: era não haver o que roubar. Este
// wrapper existe só para dar a ele blocos sobrando.
//
// O NÚMERO DE BLOCOS TEM CUSTO DOS DOIS LADOS:
//   poucos  -> desbalanceamento (o caso acima)
//   muitos  -> cada bloco paga o custo fixo do chamador. No rasterizador cada
//              faixa varre o array triBounds INTEIRO para descobrir quais
//              triângulos lhe pertencem, então dobrar as faixas dobra essa
//              varredura redundante.
// Por isso `chunks_per_thread` e `min_chunk` são explícitos: o TBB não tem como
// adivinhar o custo fixo, que está dentro da nossa lambda. Medido no cenário do
// donut (pior ângulo, rasterize em ms): K=1 -> 54,2 | K=2 -> 30,6 | K=4 -> 16,2
// | K=8 -> 13,7 | K=16 -> 15,5.
//
// `min_chunk` impede blocos menores que N unidades. Sem ele, uma viewport
// pequena com uma malha pesada geraria muitos blocos minúsculos, cada um pagando
// a varredura completa — trocando o desbalanceamento por overhead.
template <typename Function>
inline void cg_parallel_chunks_balanced(std::size_t count, Function func,
                                        unsigned chunks_per_thread = kDefaultChunksPerThread,
                                        std::size_t min_chunk = 1) {
#ifdef USE_TBB_EXECUTION
    if (AppConfig::use_tbb) {
        cg_par::tbb_impl::chunks_balanced(count, func, chunks_per_thread, min_chunk);
        return;
    }
#endif
    cg_par::native::chunks_balanced(count, func, chunks_per_thread, min_chunk);
}
