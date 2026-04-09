#ifndef MATRIX_H
#define MATRIX_H

#include "Point.hpp"
#include <vector>
#include <cassert>
#include <iostream>

namespace core {
    // Matriz precisa ser de algum tipo numérico.
    template <typename T>
    class GenericMatrix{
        public:
            int r, c; // linhas / colunas
            std::vector<std::vector<T>> data;

            GenericMatrix(int r = 0, int c = 0, T defaultT = T(), bool identity = false): r(r), c(c), data(r, std::vector<T>(c, defaultT)) {
                if(identity) for(int i = 0; i < std::min(c, r); i++) data[i][i] = 1; 
            }

            explicit GenericMatrix(const core::Point &p);

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

            friend GenericMatrix operator*(const GenericMatrix &a, const GenericMatrix &b){
                assert(a.c == b.r);
                GenericMatrix result(a.r, b.c);
                for(int i = 0; i<result.r; i++){
                    for(int j = 0; j<result.c; j++){
                        for(int k = 0; k < a.c; k++){
                            result[i][j] += a[i][k] * b[k][j];
                        }
                    }
                }
                return result;
            }

            friend core::Point operator*(const GenericMatrix &a, const Point &b){
                GenericMatrix<T> r = a * GenericMatrix<T>(b);
                return core::Point(r);
            }

            GenericMatrix& operator*=(const GenericMatrix& b) {
                *this = (*this) * b;
                return *this;
            }

            /* cout<<Matrix (xd) */
            friend std::ostream &operator<<(std::ostream &os, const GenericMatrix &m) {
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
    GenericMatrix<T>::GenericMatrix(const core::Point &p): r(4), c(1), data(4, std::vector<float>(1)){ // retorna a matriz de um ponto, utilizada nas transformações
        data[0][0] = p.x;
        data[1][0] = p.y;
        data[2][0] = p.z;
        data[3][0] = 1; // homogeneous w
    }
}



#endif // MATRIX_H