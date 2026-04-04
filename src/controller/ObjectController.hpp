#ifndef OBJECT_CONTROLLER_HPP
#define OBJECT_CONTROLLER_HPP

#include "EntityManager.hpp"
#include "Matrix.hpp"
#include <unordered_map>
#include <unordered_set>

struct Transformation {
    core::Matrix<int> matrix;
    std::vector<char*> description; // For GUI display purposes
};

class ObjectController {
private:
    EntityManager& entityManager;

    std::unordered_set<long long> selected_ids;
    //std::unordered_map<core::Matrix<int>, long long> transformation_buffer;
    std::unordered_map<long long, std::vector<Transformation>> transformation_buffer;

public:
    ObjectController(EntityManager& em) : entityManager(em) {}

    void SetSelectedIDs(const std::unordered_set<long long>& ids) {
        selected_ids = ids;
    }
    const std::vector<char*> GetTransformationBufferNames() {
        // Fazer método para retornar a intercecção de todas as transformações em comum de todos os objetos selecionados.
        std::vector<char*> v;
        return v; // placeholder
    }

    void HandleAddScaling(float x, float y);
    void HandleAddTranslation(float x, float y);
    void HandleAddRotation(float x, float y, float angle);
    void ApplyTransformations();
};

#endif // OBJECT_CONTROLLER_HPP