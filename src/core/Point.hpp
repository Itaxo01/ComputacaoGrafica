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
    static constexpr float EPS = 1e-9;

    class Point: public Shape{
        public:
            float x, y, z;
            Point(): x(0), y(0), z(0) {
                type = core::ShapeType::POINT;
            }
            Point(float x, float y, float z = 0.0f): x(x), y(y), z(z){
                type = core::ShapeType::POINT;
            }
            Point(const std::pair<float, float> &p): x(p.first), y(p.second), z(0.0f){
                type = core::ShapeType::POINT;
            }
            Point(const std::tuple<float, float, float> &p) {
                auto [x, y, z] = p;
                this->x = x, this->y = y, this->z = z;
                type = core::ShapeType::POINT;
            }

            friend Point operator+(const Point &p, const Point &q) {return Point(p.x + q.x, p.y + q.y, p.z + q.z);}
            friend Point operator-(const Point &p, const Point &q) {return Point(p.x - q.x, p.y - q.y, p.z - q.z);}
            friend Point operator*(const Point &p, const float k) {return Point(p.x * k, p.y * k, p.z * k);}
            
            friend Point operator/(const Point &p, const float k) {return Point(p.x / k, p.y / k, p.z / k);}
            Point operator+=(const Point &q) {this->x+=q.x; this->y+=q.y; this->z+=q.z; return *this;}
            Point operator-=(const Point &q) {this->x-=q.x; this->y-=q.y; this->z-=q.z; return *this;}
            Point operator*=(const float k) {this->x*=k; this->y*=k; this->z*=k; return *this;}
            Point operator/=(const float k) {this->x/=k; this->y/=k; this->z/=k; return *this;}
            
            bool operator==(const Point &q) const {return (std::abs(this->x-q.x) < EPS && std::abs(this->y - q.y) < EPS && std::abs(this->z - q.z) < EPS);}
            bool operator!=(const Point &q) const {return !(*this == q);}
            
            bool operator<(const Point &q) const {
                if (std::abs(this->x - q.x) > EPS) return this->x < q.x;
                if (std::abs(this->y - q.y) > EPS) return this->y < q.y;
                if (std::abs(this->z - q.z) > EPS) return this->z < q.z;
                return false;
            }
            
            bool operator<=(const Point &q) const {return *this < q || *this == q;}
            bool operator>=(const Point &q) const {return !(*this < q);}
            bool operator>(const Point &q) const {return !(*this < q) && !(*this == q);}

            // retorna o maior y, se iguais retorna o mais a esquerda
            friend Point max_y(const Point &p, const Point &q){
                return p.y > q.y ? p : (q.y > p.y ? q : (p.x < q.x ? p : q));
            }

            friend float dot(const Point &p, const Point&q) {
                return p.x * q.x + p.y * q.y + p.z * q.z;   
            }
            friend float dist2(const Point&p, const Point&q) {return dot(p-q, p-q);}
            friend float dist(const Point&p, const Point&q) {return sqrtl(dist2(p, q));}
            
            // 3D Cross Product
            friend Point cross(const Point &p, const Point &q) {
                return Point(p.y * q.z - p.z * q.y, p.z * q.x - p.x * q.z, p.x * q.y - p.y * q.x);
            }
            // 2D Cross Product equivalent (useful for 2D orientation logic)
            friend float cross2D(const Point &p, const Point &q) {return p.x * q.y - p.y * q.x;}

            friend float proj(const Point&p, const Point&q) {return dot(p, q) / (dist(p, q));}
            
            friend int orientation(const Point&p, const Point&q) {
                float o = cross2D(p, q);
                if (o < -EPS) return -1; // clockwise (Direita)
                if (o > EPS) return 1;  // counter clockwise (Esquerda)
                return 0;       // collinear
            }
            friend bool collinear(const Point&p, const Point&q) {return orientation(p, q) == 0;}
            
            static bool ccw_cmp(const Point&p, const Point&q) {
                int o = orientation(p, q);
                if(o == 0){
                    return dot(p, p) < dot(q, q); 
                }
                return o > 0; 
            };
            
            static bool cw_cmp(const Point&p, const Point&q) {
                int o = orientation(p, q);
                if(o == 0){
                    return dot(p, p) < dot(q, q); 
                }
                return o < 0; 
            }

            std::tuple<float, float, float> expand() const {
                return std::make_tuple(x, y, z);
            }

            std::tuple<float, float, float> anchorPoint() const override {
                return expand();
            }
            
            friend std::ostream &operator<<(std::ostream &os, const Point &p) {
                os <<"("<< p.x << ", " << p.y << ", " << p.z << ")";
                return os;
            }


            std::string coords(bool p3d) const {
                std::string r = "(" + format(x, 2) + ", " + format(y, 2);
                if(p3d) r += + ", " + format(z, 2);
                r +=  ")";
                return r;
            }

            ObjectDetails GetObjectDetails(long long id, bool p3d = false) const {
                ObjectDetails details;
                details.type = "Point";
                details.id = std::to_string(id);
                details.name = this->getName();
                details.color = this->getColor();
                details.points = "[" + coords(p3d) + "]";
                return details;
            }

            std::tuple<float, float, float> centerPoint() const override {
                return std::make_tuple(x, y, z);
            }
    };
 
}
#endif // POINT_H