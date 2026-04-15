#ifndef BEZIER_CURVE_H
#define BEZIER_CURVE_H

#include <ostream>
#include "Point.hpp"
#include "Shape.hpp"
#include <vector>

namespace core{
    class BezierCurve: public Shape{
        public:
        std::vector<core::Point> points;
        BezierCurve(std::vector<core::Point> &data): points(data) {
            type = ShapeType::BEZIER_CURVE;
        }

        std::tuple<float, float, float> anchorPoint() const override{
            core::Point p = points.front();
            for(long unsigned int i = 1; i<points.size(); i++){
                p = max_y(p, points[i]);
            }
            return p.expand();
        }


        friend std::ostream &operator<<(std::ostream &os, const BezierCurve &l) {
            os <<"[";
            for(int i = 0; i<(int)l.points.size(); i++){
                if(i) os<<" - ";
                os<<l.points[i];
            }
            os<<"]";
            return os;
        }

        ObjectDetails GetObjectDetails(long long id, bool p3d = false) const {
            ObjectDetails details;
            details.type = "Bezier Curve";
            details.id = std::to_string(id);
            details.name = this->getName();
            details.color = this->getColor();
            
            std::string pts = "[";
            for (size_t i = 0; i < points.size(); ++i) {
                pts += points[i].coords(p3d);
                if (i < points.size() - 1) pts += "\n";
            }
            pts += "]";
            details.points = pts;
            return details;
        }

        std::tuple<float, float, float> centerPoint() const override {
            float sum_x = 0, sum_y = 0, sum_z = 0;
            for (const auto &point : points) {
                sum_x += point.x;
                sum_y += point.y;
                sum_z += point.z;
            }
            return std::make_tuple(sum_x / points.size(), sum_y / points.size(), sum_z / points.size());
        }
    };
}
#endif // BEZIER_CURVE_H