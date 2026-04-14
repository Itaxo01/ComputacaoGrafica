#ifndef LINE_H
#define LINE_H

#include <ostream>
#include "Point.hpp"
#include "Shape.hpp"

namespace core{
    class Line: public Shape{
        public:
        core::Point a, b;
        Line(){
            type = ShapeType::LINE;
        }
        Line(core::Point &a, core::Point &b): a(a), b(b) {
            type = ShapeType::LINE;
        }

        std::tuple<float, float, float> anchorPoint() const override{
            auto p = max_y(a, b);
            return p.expand();
        }

        friend std::ostream &operator<<(std::ostream &os, const Line &l) {
            os <<"["<< l.a << " -- " << l.b<<"]";
            return os;
        }
        ObjectDetails GetObjectDetails(long long id, bool p3d = false) const {
            ObjectDetails details;
            details.type = "Line";
            details.id = std::to_string(id);
            details.name = this->getName();
            details.color = this->getColor();
            details.points = "[" + a.coords(p3d) + '\n' + b.coords(p3d) + "]";
            return details;
        }
        std::tuple<float, float, float> centerPoint() const override {
            return std::make_tuple((a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f, (a.z + b.z) / 2.0f);
        }
    };
    
}
#endif // LINE_H