#ifndef OBJECT_CONTROLLER_HPP
#define OBJECT_CONTROLLER_HPP

#include "EntityManager.hpp"
#include "Mat4.hpp"
#include "Point.hpp"
#include "Transformations.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Tipo de transformação paramétrica. Guardar o tipo + parâmetros (em vez de só a
// matriz final) permite reavaliar a transformação numa fração "alpha" do caminho,
// o que é o que torna possível animar a transição suavemente.
enum class TransformKind { SCALE, TRANSLATE, ROTATE };

struct Transformation {
    long long id;
    core::mat4 matrix;       // matriz final (equivalente a matrixAt(1.0f))
    char* description;       // For GUI display purposes
    TransformKind kind = TransformKind::TRANSLATE;
    core::Point vec;         // translação: delta | escala: fatores | rotação: centro
    core::Point axis{0, 0, 1}; // rotação: eixo (default Z, usado no modo 2D)
    float angle = 0.0f;      // rotação: ângulo em graus

    // Matriz da transformação avaliada em "alpha" ∈ [0,1]: alpha=0 → identidade,
    // alpha=1 → transformação completa. Interpola os parâmetros (não a matriz).
    core::mat4 matrixAt(float alpha) const;
};

// Animação em curso de um objeto: parte da transformação "base" (a que o objeto
// tinha quando a animação começou) e reaplica a combinação de "steps" avaliada
// num alpha crescente até durar "duration" segundos.
struct ObjectAnimation {
    core::mat4 base;
    std::vector<Transformation> steps;
    float duration = 0.0f;
    float elapsed = 0.0f;
    bool  loop = false;   // se true, reinicia ao terminar (ex.: donut girando para sempre)
};

class ObjectController {
private:
    EntityManager& entityManager;

    std::unordered_set<long long> selected_ids;
    std::vector<Transformation> selected_transformations;   // Transformar em unordered_set no futuro
    std::unordered_map<long long, std::vector<Transformation>> transformation_buffer;

    long long id_counter = 0; // For generating unique IDs for transformations

    std::vector<Transformation> tranformation_intersection;
    void TransformationIntersection();

    std::unordered_map<long long, ObjectAnimation> active_animations;
public:
    ObjectController(EntityManager& em) : entityManager(em) {}

    void SetSelectedIDs(const std::unordered_set<long long>& ids) {
        selected_ids = ids;
    }
    void SetSelectedTransfomations(const std::unordered_set<int>& indexes) {
        TransformationIntersection(); // mudar de lugar depois
        selected_transformations.clear();
        for (int i : indexes) {
            selected_transformations.push_back(tranformation_intersection[i]);
        }
    }
    core::mat4 GetSelectedTransformationMatrix() {
        return selected_transformations.empty() ? core::mat4(true) : selected_transformations[0].matrix;
    }
    const std::vector<char*> GetTransformationBufferNames();

    std::tuple<float, float, float> GetSelectedObjectsCenter() {
        for (long long id : selected_ids) {
            return entityManager.getObject(id).centerPoint().expand();
        }
        return std::make_tuple(0.0f, 0.0f, 0.0f);
    }
    
    void HandleAddScaling(float x, float y, float z = 1.0f);
    void HandleAddTranslation(float x, float y, float z = 0.0f);
    // Rotação de "angle" graus em torno do eixo (ax, ay, az), centrada em (cx, cy, cz).
    void HandleAddRotation(float cx, float cy, float cz,
                           float ax, float ay, float az, float angle);

    // duration <= 0 → aplica instantaneamente (comportamento antigo).
    // duration  > 0 → anima a transição ao longo de "duration" segundos.
    // loop = true   → repete a animação indefinidamente (transição contínua).
    void ApplyTransformations(float duration = 0.0f, bool loop = false);

    // Avança as animações em curso. Deve ser chamado a cada frame com o dt (s).
    void Update(float dt);
    bool HasActiveAnimations() const { return !active_animations.empty(); }
    void StopAnimations() { active_animations.clear(); }

    // Importa transformações a partir de um script de texto e aplica-as aos
    // objetos selecionados. Gramática (uma instrução por linha, '#' = comentário):
    //   duration <s> | loop [on|off] |
    //   translate <x> <y> [z] | scale <sx> <sy> [sz] |
    //   rotate <ang> [axis <ax> <ay> <az>] [around itself|origin|point <x> <y> <z>]
    // Devolve "" em caso de sucesso, ou uma mensagem de erro.
    std::string ApplyTransformationScript(const std::string& text);
    std::string ApplyTransformationScriptFile(const std::string& path);
};

#endif // OBJECT_CONTROLLER_HPP