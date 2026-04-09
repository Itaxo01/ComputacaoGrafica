#include "ObjectController.hpp"

void ObjectController::HandleAddScaling(float x, float y) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Scaling: (%.1f, %.1f)", x, y);
    for (long long id : selected_ids) {
        Transformation t = {id_counter, core::getScalingMatrix(x, y), buffer};
        transformation_buffer[id].push_back(t);
    }
    id_counter++;
}

void ObjectController::HandleAddTranslation(float x, float y) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Translation: (%.1f, %.1f)", x, y);
    for (long long id : selected_ids) {
        Transformation t = {id_counter, core::getTranslationMatrix(x, y), buffer};
        transformation_buffer[id].push_back(t);
    }
    id_counter++;
}

void ObjectController::HandleAddRotation(float x, float y, float angle) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Rotation: (%.1f, %.1f, %.1f)", x, y, angle);
    for (long long id : selected_ids) {
        core::Point p = {x, y};
        Transformation t = {id_counter, core::getRotationMatrixCenteredAt(angle, p), buffer};
        transformation_buffer[id].push_back(t);
    }
    id_counter++;
}

void ObjectController::ApplyTransformations() {
   for (long long id : selected_ids) {
        core::mat4 final_matrix(true);
        for (const auto& t : transformation_buffer[id]) {
            final_matrix = t.matrix * final_matrix;
        }
        entityManager.ApplyTransformation(id, final_matrix);
        transformation_buffer[id].clear();
    }
}

void ObjectController::TransformationIntersection() {
    tranformation_intersection.clear();
    if (selected_ids.empty()) {
        return;
    }

    std::vector<Transformation> base = transformation_buffer[*selected_ids.begin()];
    std::vector<Transformation> intersection;

    for (long long id : selected_ids) {
        if (id == *selected_ids.begin()) {
            continue;
        }

        intersection.clear();
        for (const auto& t1 : base) {
            for (const auto& t2 : transformation_buffer[id]) {
                if (t1.id == t2.id) { // ADICIONAR ID PARA TRANSFOMAÇÃO
                    intersection.push_back(t1);
                    //break;
                }
            }
        }
        base = intersection;
    }

    tranformation_intersection = base;
}

// Por enquanto está quadrático. Depois fazer algoritmo melhor usando hash.
// Reaproveitar metodo Transformation Intersection depois também.
const std::vector<char*> ObjectController::GetTransformationBufferNames() {
    if (selected_ids.empty()) {
        return {};
    }

    std::vector<Transformation> base = transformation_buffer[*selected_ids.begin()];
    std::vector<Transformation> intersection;

    for (long long id : selected_ids) {
        if (id == *selected_ids.begin()) {
            continue;
        }

        intersection.clear();
        for (const auto& t1 : base) {
            for (const auto& t2 : transformation_buffer[id]) {
                if (t1.id == t2.id) { // ADICIONAR ID PARA TRANSFOMAÇÃO
                    intersection.push_back(t1);
                    break;
                }
            }
        }
        base = intersection;
    }

    std::vector<char*> names;
    for (const auto& t : base) {
        names.push_back(t.description);
    }

    return names;
}