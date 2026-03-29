#ifndef POINT_H
#define POINT_H

#include <ostream>
#include <cmath>
#include "Shape.hpp"

/* Points are vectors, we just assume that they start at the origin(0,0). We will have to decide where to make a separate Vector class or we just add a Point to the Point class, referencing the translation of that point (the vector origin). 
* And yes, everything is implemented on the header. Apparently that's a good practice for this kind of class.

Essa classe não precisava ser tão grande, é só pq eu já tinha a estrutura pronta (livro do time de maratona de programação).
*/
namespace core{
    class Point: public Shape{
        public:
            static constexpr float EPS = 1e-9; // This is our threshold for something like x == y on floats, there could be a tiny error that the EPS will handle
            float x, y;
            Point(float x = 0, float y = 0): x(x), y(y){
                type = core::ShapeType::POINT;
            }
            Point(std::pair<float, float> p): x(p.first), y(p.second){
                type = core::ShapeType::POINT;
            }

            friend Point operator+(const Point &p, const Point &q) {return Point(p.x + q.x, p.y + q.y);}
            friend Point operator-(const Point &p, const Point &q) {return Point(p.x - q.x, p.y - q.y);}
            friend Point operator*(const Point &p, const float k) {return Point(p.x * k, p.y * k);}
            friend Point operator/(const Point &p, const float k) {return Point(p.x / k, p.y / k);}
            Point operator+=(const Point &q) {return Point(this->x+q.x, this->y+q.y);}
            Point operator-=(const Point &q) {return Point(this->x-q.x, this->y-q.y);}
            Point operator*=(const float k) {return Point(this->x*k, this->y*k);}
            Point operator/=(const float k) {return Point(this->x/k, this->y/k);}
            /**Compares x first, then y.*/
            bool operator==(const Point &q) {return (std::abs(this->x-q.x) < EPS && std::abs(this->y - q.y) < EPS);}
            /**Compares x first, then y.*/
            bool operator<(const Point &q) {return (std::abs(q.x - this->x) > EPS || (std::abs(this->x - q.x) < EPS && std::abs(q.y - this->y) > EPS));}
            /*Just the two above need to be implemented, the others will be derivated. This is as fast as anything, everything that is declared on the header is interpreted as a inline.*/
            
            /**Compares x first, then y.*/
            bool operator<=(const Point &q) {return *this < q || *this == q;}
            /**Compares x first, then y.*/
            bool operator>=(const Point &q) {return !(*this < q);}
            /**Compares x first, then y.*/
            bool operator>(const Point &q) {return !(*this < q) && !(*this == q);}

            // retorna o maior y, se iguais retorna o mais a esquerda
            friend Point max_y(const Point &p, const Point &q){
                return p.y > q.y ? p : (q.y > p.y ? q : (p.x < q.x ? p : q));
            }

            /**Dot product (Produto escalar ou interno). Quite useful. Can and will be used to measure the angle between points as vectors (by the definition A dot B = |A|*|B|*cos(theta)) and projections.*/
            friend float dot(const Point &p, const Point&q) {
                return p.x * q.x + p.y * q.y;   
            }
            /**Distance between points without square root */
            friend float dist2(const Point&p, const Point&q) {return dot(p-q, p-q);}
            /**Distance between points with square root */
            friend float dist(const Point&p, const Point&q) {return sqrtl(dist2(p, q));}
            /* Cross product, useful to find the orientation (This is a 3D operation, but it's simplified with Z = 0)*/
            friend float cross(const Point &p, const Point &q) {return p.x * q.y - p.y * q.x;}
            /**Projection of one vector in to another*/
            friend float proj(const Point&p, const Point&q) {return dot(p, q) / (dist(p, q));}
            /** @returns orientation between two vectors (to the right, to the left or collinear)*/
            friend int orientation(const Point&p, const Point&q) {
                float o = cross(p, q);
                if (o < -EPS) return -1; // clockwise (Direita)
                if (o > EPS) return 1;  // counter clockwise (Esquerda)
                return 0;       // collinear
            }
            /* Tells if two points are collinear*/
            friend bool collinear(const Point&p, const Point&q) {return orientation(p, q) == 0;}
            /* Useful for sorting counter clockwise*/
            static bool ccw_cmp(const Point&p, const Point&q) {
                int o = orientation(p, q);
                if(o == 0){
                    return dot(p, p) < dot(q, q); // Em caso de colinear, o primeiro é o mais próximo da origem
                }
                return o > 0; // o > 0 significa que Q está a esquerda de P, ou seja, P está mais para a direita e vem antes no ccw.
            };
            /* Useful for sorting clockwise*/
            static bool cw_cmp(const Point&p, const Point&q) {
                int o = orientation(p, q);
                if(o == 0){
                    return dot(p, p) < dot(q, q); // Em caso de colinear, o primeiro é o mais próximo da origem
                }
                return o < 0; // o < 0 significa que Q está a direita de P, ou seja, P está mais para a esquerda e vem antes no cw.
            }

            std::pair<float, float> anchorPoint() const{
                return std::make_pair(x, y);
            }
            
            /* cout<<Point */
            friend std::ostream &operator<<(std::ostream &os, const Point &p) {
                os <<"("<< p.x << ", " << p.y<<")";
                return os;
            }
    };
    
}
#endif // POINT_H