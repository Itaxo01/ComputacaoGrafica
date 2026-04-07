#ifndef OBJECT_CONTROLLER_HPP
#define OBJECT_CONTROLLER_HPP

#include "EntityManager.hpp"
#include "Matrix.hpp"
#include "Transformations.hpp"
#include <unordered_map>
#include <unordered_set>

struct Transformation {
    long long id;
    core::Matrix<float> matrix;
    char* description; // For GUI display purposes
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
    core::Matrix<float> GetSelectedTransformationMatrix() {
        return selected_transformations.empty() ? core::Matrix<float>(4, 4, 0.0f, true) : selected_transformations[0].matrix;
    }
    const std::vector<char*> GetTransformationBufferNames();

    std::tuple<float, float, float> GetSelectedObjectsCenter() {
        for (long long id : selected_ids) {
            auto &obj = entityManager.getObject(id);
            return obj.centerPoint(); // Por enquanto retorna o centro de apenas um objeto...
        }
        return std::make_tuple(0.0f, 0.0f, 0.0f); // Default return if no objects are selected
    }
    
    void HandleAddScaling(float x, float y);
    void HandleAddTranslation(float x, float y);
    void HandleAddRotation(float x, float y, float angle);
    void ApplyTransformations();
};

#endif // OBJECT_CONTROLLER_HPP