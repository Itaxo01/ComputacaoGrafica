// ESPECIALIZAÇÃO DE MATRIZ: apenas 4x4

#ifndef MAT4_H
#define MAT4_H

#include "Point.hpp"
#include <cstring>
#ifdef DEBUG
    #include <cassert>
    #include <iostream>
#endif

namespace core {
    // Matriz precisa ser de algum tipo numérico.
    class mat4{
        public:
            float data[16] = {0.0f};

            mat4(bool identity = false) {
                if(identity) {
                    data[0] = 1.0f, data[5] = 1.0f, data[10] = 1.0f, data[15] = 1.0f;
                }
            }

            inline float *operator[](int i){ return &data[i*4];}
            inline const float* operator[](int i) const { return &data[i*4]; }

            friend mat4 operator*(const mat4 &a, const mat4 &b){
                mat4 result;
                for(int i = 0; i<4; i++){
                    for(int j = 0; j<4; j++){
                        float sum = 0.0f;
                        for(int k = 0; k < 4; k++){
                            sum += a.data[i*4 + k] * b.data[k*4 + j];
                        }
                        result.data[i*4 + j] = sum;
                    }
                }
                return result;
            }

            // Isso está extensivo para maximizar a velocidade
            friend core::Point operator*(const mat4 &m, const Point &p){
                const float x = m.data[0]*p.x + m.data[1]*p.y + m.data[2]*p.z  + m.data[3];
                const float y = m.data[4]*p.x + m.data[5]*p.y + m.data[6]*p.z  + m.data[7];
                const float z = m.data[8]*p.x + m.data[9]*p.y + m.data[10]*p.z + m.data[11];

                // Caminho afim (última linha = 0,0,0,1): vale para as matrizes de
                // modelo, a NCS e a de viewport, ou seja, a grande maioria das
                // multiplicações do pipeline. Só a projeção perspectiva precisa da
                // divisão, e evitá-la aqui economiza 3 divisões por vértice (divss
                // custa ~10 ciclos de latência no Zen2, contra ~0.5 do mulss).
                if (m.data[12] == 0.0f && m.data[13] == 0.0f &&
                    m.data[14] == 0.0f && m.data[15] == 1.0f)
                    return core::Point(x, y, z);
 
                float w = m.data[12]*p.x + m.data[13]*p.y + m.data[14]*p.z + m.data[15];

                // Preventing Div-By-Zero
                if (w == 0.0f) w = 1.0f;

                const float inv_w = 1.0f / w; // uma divisão em vez de três
                return core::Point(x * inv_w, y * inv_w, z * inv_w);
            }

            mat4& operator*=(const mat4& b) {
                *this = (*this) * b;
                return *this;
            }

            /* cout<<Matrix (xd) */
            friend std::ostream &operator<<(std::ostream &os, const mat4 &m) {
                #ifdef DEBUG
                for(int i = 0; i<4; i++){
                    os <<"(";
                    for(int j = 0; j<4; j++){
                        if(j) std::cout<<", ";
                        os<<m[i][j];
                    }
                    os <<")";
                }
                #endif
                return os;
            }
    };
}

#endif // MAT4_H