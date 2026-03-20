#ifndef LINE_H
#define LINE_H

#include <ostream>
#include "Point.hpp"

namespace core{
    class Line{
        core::Point a, b;
        Line(core::Point &a, core::Point &b): a(a), b(b) {}


        friend std::ostream &operator<<(std::ostream &os, const Line &l) {
            os <<"["<< l.a << " -- " << l.b<<"]";
            return os;
        }
    };
    
}
#endif // LINE_H