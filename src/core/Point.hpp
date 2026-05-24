#ifndef POINT_H
#define POINT_H

#include <ostream>
#include <cmath>
#include "AppConfig.hpp"

namespace core {
    static constexpr float EPS = 1e-9;

    class Point {
    public:
        float x = 0, y = 0, z = 0;

        Point() = default;
        Point(float x, float y, float z = 0.0f) : x(x), y(y), z(z) {}
        Point(const std::pair<float, float> &p) : x(p.first), y(p.second), z(0.0f) {}
        Point(const std::tuple<float, float, float> &p) {
            auto [px, py, pz] = p;
            x = px; y = py; z = pz;
        }

        friend Point operator+(const Point &p, const Point &q) { return Point(p.x+q.x, p.y+q.y, p.z+q.z); }
        friend Point operator-(const Point &p, const Point &q) { return Point(p.x-q.x, p.y-q.y, p.z-q.z); }
        friend Point operator*(const Point &p, const float k)  { return Point(p.x*k,   p.y*k,   p.z*k);   }
        friend Point operator/(const Point &p, const float k)  { return Point(p.x/k,   p.y/k,   p.z/k);   }

        Point operator+=(const Point &q) { x+=q.x; y+=q.y; z+=q.z; return *this; }
        Point operator-=(const Point &q) { x-=q.x; y-=q.y; z-=q.z; return *this; }
        Point operator*=(const float k)  { x*=k;   y*=k;   z*=k;   return *this; }
        Point operator/=(const float k)  { x/=k;   y/=k;   z/=k;   return *this; }

        bool operator==(const Point &q) const { return (std::abs(x-q.x)<EPS && std::abs(y-q.y)<EPS && std::abs(z-q.z)<EPS); }
        bool operator!=(const Point &q) const { return !(*this == q); }
        bool operator<(const Point &q) const {
            if (std::abs(x-q.x) > EPS) return x < q.x;
            if (std::abs(y-q.y) > EPS) return y < q.y;
            if (std::abs(z-q.z) > EPS) return z < q.z;
            return false;
        }
        bool operator<=(const Point &q) const { return *this < q || *this == q; }
        bool operator>=(const Point &q) const { return !(*this < q); }
        bool operator>(const Point &q) const  { return !(*this < q) && !(*this == q); }

        friend Point max_y(const Point &p, const Point &q) {
            return p.y > q.y ? p : (q.y > p.y ? q : (p.x < q.x ? p : q));
        }

        friend float dot(const Point &p, const Point &q) { return p.x*q.x + p.y*q.y + p.z*q.z; }
        friend float dist2(const Point &p, const Point &q) { return dot(p-q, p-q); }
        friend float dist(const Point &p, const Point &q)  { return sqrtl(dist2(p, q)); }

        friend Point cross(const Point &p, const Point &q) {
            return Point(p.y*q.z - p.z*q.y, p.z*q.x - p.x*q.z, p.x*q.y - p.y*q.x);
        }
        friend float cross2D(const Point &p, const Point &q) { return p.x*q.y - p.y*q.x; }
        friend float proj(const Point &p, const Point &q) { return dot(p,q) / dist(p,q); }

        friend int orientation(const Point &p, const Point &q) {
            float o = cross2D(p, q);
            if (o < -EPS) return -1;
            if (o >  EPS) return  1;
            return 0;
        }
        friend bool collinear(const Point &p, const Point &q) { return orientation(p,q) == 0; }

        static bool ccw_cmp(const Point &p, const Point &q) {
            int o = orientation(p, q);
            return o == 0 ? dot(p,p) < dot(q,q) : o > 0;
        }
        static bool cw_cmp(const Point &p, const Point &q) {
            int o = orientation(p, q);
            return o == 0 ? dot(p,p) < dot(q,q) : o < 0;
        }

        std::tuple<float, float, float> expand() const { return {x, y, z}; }

        std::string coords() const {
            std::string r = "(" + format(x, 2) + ", " + format(y, 2);
            if (AppConfig::is3d) r += ", " + format(z, 2);
            r += ")";
            return r;
        }

        friend std::ostream &operator<<(std::ostream &os, const Point &p) {
            os << "(" << p.x << ", " << p.y << ", " << p.z << ")";
            return os;
        }
    };
}

#endif // POINT_H
