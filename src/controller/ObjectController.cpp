#include "ObjectController.hpp"

void ObjectController::HandleAddScaling(float x, float y) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Scaling: (%.1f, %.1f)", x, y);
    for (long long id : selected_ids) {
        //transformation_buffer[core::Matrix<int>::Scaling(x, y)] = id;
        ;//transformation_buffer_names[buffer] = id;
    }
}

void ObjectController::HandleAddTranslation(float x, float y) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Translation: (%.1f, %.1f)", x, y);
    for (long long id : selected_ids) {
        //transformation_buffer[core::Matrix<int>::Translation(x, y)] = id;
        ;//transformation_buffer_names[buffer] = id;
    }
}

void ObjectController::HandleAddRotation(float x, float y, float angle) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Rotation: (%.1f, %.1f, %.1f)", x, y, angle);
    for (long long id : selected_ids) {
        //transformation_buffer[core::Matrix<int>::Rotation(x, y, angle)] = id;
        ;//transformation_buffer_names[buffer] = id;
    }
}

void ObjectController::ApplyTransformations() {
    // Implementation for applying transformations
}