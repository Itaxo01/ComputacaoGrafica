#ifndef MATRIX_H
#define MATRIX_H

#include "Point.hpp"
#include <vector>
#include <cassert>
#include <iostream>

namespace core {
    // Matriz precisa ser de algum tipo numérico.
    template <typename T>
    class Matrix{
        public:
            int r, c; // linhas / colunas
            std::vector<std::vector<T>> data;

            Matrix(int r = 0, int c = 0, T defaultT = T(), bool identity = false): r(r), c(c), data(r, std::vector<T>(c, defaultT)) {
                if(identity) for(int i = 0; i < std::min(c, r); i++) data[i][i] = 1; 
            }

            explicit Matrix(const core::Point &p);

            // retornar por referência só é seguro quando o objeto existe fora do método
            // apenas preciso dar override do operator uma vez, se chamar Matrix[i][j], o j é resolvido pelo vetor de Matrix[i].
            std::vector<T> &operator[](int i){
                assert(i >= 0 && i < r);
                return data[i];
            }
            const std::vector<T>& operator[](int i) const {
                assert(i >= 0 && i < r);
                return data[i];
            }

            friend Matrix operator*(const Matrix &a, const Matrix &b){
                assert(a.c == b.r);
                Matrix result(a.r, b.c);
                for(int i = 0; i<result.r; i++){
                    for(int j = 0; j<result.c; j++){
                        for(int k = 0; k < a.c; k++){
                            result[i][j] += a[i][k] * b[k][j];
                        }
                    }
                }
                return result;
            }

            friend core::Point operator*(const Matrix &a, const Point &b){
                Matrix<T> r = a * Matrix<T>(b);
                return core::Point(r);
            }

            Matrix& operator*=(const Matrix& b) {
                *this = (*this) * b;
                return *this;
            }

            /* cout<<Matrix (xd) */
            friend std::ostream &operator<<(std::ostream &os, const Matrix &m) {
                for(int i = 0; i<m.r; i++){
                    os <<"(";
                    for(int j = 0; j<m.c; j++){
                        if(j) std::cout<<", ";
                        os<<m[i][j];
                    }
                    os <<")";
                }
                return os;
            }
    };

    // As declarações foram para o final para lidar com cross reference por forward declaration.
    template <typename T>
    Matrix<T>::Matrix(const core::Point &p): r(4), c(1), data(4, std::vector<float>(1)){ // retorna a matriz de um ponto, utilizada nas transformações
        data[0][0] = p.x;
        data[1][0] = p.y;
        data[2][0] = p.z;
        data[3][0] = 1; // homogeneous w
    }

    inline Point::Point(const Matrix<float>& m) : x(0), y(0), z(0) {
        assert(m.r >= 3 && m.c == 1); // Must be a column vector [x, y, z, (w)]
        
        float w = (m.r == 4) ? m.data[3][0] : 1.0f;
        
        if (w != 0.0f && w != 1.0f) {
            x = m.data[0][0] / w;
            y = m.data[1][0] / w;
            z = m.data[2][0] / w;
        } else {
            x = m.data[0][0];
            y = m.data[1][0];
            z = m.data[2][0];
        }
        
        type = core::ShapeType::POINT;
    }
}



#endif // MATRIX_H