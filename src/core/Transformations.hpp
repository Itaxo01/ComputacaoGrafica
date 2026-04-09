#pragma once // substitui o #ifndef ..
#include "Mat4.hpp"
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

    inline mat4 getTranslationMatrix(float tx, float ty, float tz = 0.0f){
        mat4 m(true);
        m[0][3] = tx;
        m[1][3] = ty;
        m[2][3] = tz;
        return m;
    }
    inline mat4 getScalingMatrix(float sx, float sy, float sz = 1.0f){
        mat4 m(true);
        m[0][0] = sx;
        m[1][1] = sy;
        m[2][2] = sz;
        return m;
    }
    inline mat4 getRotationMatrixZ(float degrees){
        mat4 m(true);
        float rad = toRadians(degrees);
        float c = std::cos(rad);
        float s = std::sin(rad);
        
        m[0][0] = c;
        m[0][1] = -s;
        m[1][0] = s;
        m[1][1] = c;
        return m;
    }
    
    inline mat4 getRotationMatrixX(float degrees){
        mat4 m(true);
        float rad = toRadians(degrees);
        float c = std::cos(rad);
        float s = std::sin(rad);
        
        m[1][1] = c;
        m[1][2] = s;
        m[2][1] = -s;
        m[2][2] = c;
        return m;
    }
    inline mat4 getRotationMatrixY(float degrees){
        mat4 m(true);
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
    inline mat4 getRotationMatrix2D(float degrees){
        return getRotationMatrixZ(degrees);
    }

    // No momento essa função é utilizada para 2d apenas.
    inline mat4 getRotationMatrixCenteredAt(float degrees, core::Point &p){
        return getTranslationMatrix(p.x, p.y) *
                getRotationMatrixZ(degrees) *
                getTranslationMatrix(-p.x, -p.y);
    }
}