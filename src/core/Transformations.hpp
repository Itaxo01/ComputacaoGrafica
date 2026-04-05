#pragma once // substitui o #ifndef ..
#include "Matrix.hpp"
#include "Point.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace core {
    // Matrizes retiradas de https://www.brainvoyager.com/bv/doc/UsersGuide/CoordsAndTransforms/SpatialTransformationMatrices.html

    inline float toRadians(float degrees) {
        return degrees * M_PI / 180.0f;
    }

    inline Matrix<float> getTranslationMatrix(float tx, float ty, float tz = 0.0f){
        Matrix<float> m(4, 4, 0.0f, true);
        m[0][3] = tx;
        m[1][3] = ty;
        m[2][3] = tz;
        return m;
    }
    inline Matrix<float> getScalingMatrix(float sx, float sy, float sz = 1.0f){
        Matrix<float> m(4, 4, 0.0f, true);
        m[0][0] = sx;
        m[1][1] = sy;
        m[2][2] = sz;
        return m;
    }
    inline Matrix<float> getRotationMatrixZ(float degrees){
        Matrix<float> m(4, 4, 0.0f, true);
        float rad = toRadians(degrees);
        float c = std::cos(rad);
        float s = std::sin(rad);
        
        m[0][0] = c;
        m[0][1] = -s;
        m[1][0] = s;
        m[1][1] = c;
        return m;
    }
    
    inline Matrix<float> getRotationMatrixX(float degrees){
        Matrix<float> m(4, 4, 0.0f, true);
        float rad = toRadians(degrees);
        float c = std::cos(rad);
        float s = std::sin(rad);
        
        m[1][1] = c;
        m[1][2] = s;
        m[2][1] = -s;
        m[2][2] = c;
        return m;
    }
    inline Matrix<float> getRotationMatrixY(float degrees){
        Matrix<float> m(4, 4, 0.0f, true);
        float rad = toRadians(degrees);
        float c = std::cos(rad);
        float s = std::sin(rad);
        
        m[0][0] = c;
        m[0][2] = -s;
        m[2][0] = s;
        m[2][2] = c;
        return m;
    }

    // O default para 2d é utilizar o eixo Z
    inline Matrix<float> getRotationMatrix2D(float degrees){
        return getRotationMatrixZ(degrees);
    }

    // No momento essa função é utilizada para 2d apenas.
    inline Matrix<float> getRotationMatrixCenteredAt(float degrees, core::Point &p){
        return getTranslationMatrix(p.x, p.y) *
                getRotationMatrixZ(degrees) *
                getTranslationMatrix(-p.x, -p.y);
    }
}