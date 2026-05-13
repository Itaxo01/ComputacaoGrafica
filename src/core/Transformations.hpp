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

    // Rodrigues' rotation formula: rotate by degrees around an arbitrary unit axis.
    inline mat4 getRotationMatrixAroundAxis(float degrees, const core::Point &axis) {
        float rad = toRadians(degrees);
        float c = std::cos(rad), s = std::sin(rad), ic = 1.0f - c;
        float ax = axis.x, ay = axis.y, az = axis.z;

        mat4 m(true);
        m[0][0] = c + ax*ax*ic;       m[0][1] = ax*ay*ic - az*s;   m[0][2] = ax*az*ic + ay*s;
        m[1][0] = ay*ax*ic + az*s;    m[1][1] = c + ay*ay*ic;      m[1][2] = ay*az*ic - ax*s;
        m[2][0] = az*ax*ic - ay*s;    m[2][1] = az*ay*ic + ax*s;   m[2][2] = c + az*az*ic;
        return m;
    }

    // Rotation around an arbitrary axis centered at point p.
    inline mat4 getRotationMatrixAroundAxisCenteredAt(float degrees, const core::Point &axis, const core::Point &p) {
        return getTranslationMatrix(p.x, p.y, p.z) *
               getRotationMatrixAroundAxis(degrees, axis) *
               getTranslationMatrix(-p.x, -p.y, -p.z);
    }
}