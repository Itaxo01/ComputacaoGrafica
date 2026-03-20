#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <cassert>

namespace core {
    template <typename T>
    class Matrix{
        int r, c; // linhas / colunas
        std::vector<std::vector<T>> data;

        Matrix(int r = 0, int c = 0, T defaultT = T()): r(r), c(c), data(r, std::vector<T>(c, defaultValue)) {}

        // retornar por referência só é seguro quando o objeto existe fora do método
        // apenas preciso dar override do operator uma vez, se chamar Matrix[i][j], o j é resolvido pelo vetor de Matrix[i].
        std::vector<T> &operator[](int i){
            assert(i >= 0 && i < r);
            return data[i];
        }
        const std::vector<T>& operator[](int i) const {
            assert(i >= 0 && i < r);
            return data[i]
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
        Matrix& operator*=(const Matrix& b) {
            *this = (*this) * b;
            return *this;
        }

        /* cout<<Matrix (xd) */
        friend std::ostream &operator<<(std::ostream &os, const Matrix &m) {
            for(int i = 0; i<m.r; i++){
                os <<"(";
                for(int j = 0; j<m.c; j++){
                    if(j) cout<<", ";
                    os<<m[i][j];
                }
                os <<")";
            }
            return os;
        }
};
}



#endif // MATRIX_H