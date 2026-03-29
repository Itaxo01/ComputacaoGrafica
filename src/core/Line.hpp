#ifndef LINE_H
#define LINE_H

#include <ostream>
#include "Point.hpp"
#include "Shape.hpp"

namespace core{
    class Line: public Shape{
        public:
        core::Point a, b;
        Line(core::Point &a, core::Point &b): a(a), b(b) {
            type = ShapeType::LINE;
        }

        std::pair<float, float> anchorPoint() const{
            auto p = max_y(a, b);
            return std::make_pair(p.x, p.y);
        }

        friend std::ostream &operator<<(std::ostream &os, const Line &l) {
            os <<"["<< l.a << " -- " << l.b<<"]";
            return os;
        }
    };
    
}
#endif // LINE_H