#ifndef WIREFRAME_H
#define WIREFRAME_H

#include <ostream>
#include "Point.hpp"
#include "Shape.hpp"
#include <vector>

namespace core{
    class Wireframe: public Shape{
        public:
        std::vector<core::Point> points;
        Wireframe(std::vector<core::Point> &data): points(data) {
            type = ShapeType::WIREFRAME;
        }

        std::tuple<float, float, float> anchorPoint() const {
            core::Point p = points.front();
            for(long unsigned int i = 1; i<points.size(); i++){
                p = max_y(p, points[i]);
            }
            return p.expand();
        }


        friend std::ostream &operator<<(std::ostream &os, const Wireframe &l) {
            os <<"[";
            for(int i = 0; i<(int)l.points.size(); i++){
                if(i) os<<" - ";
                os<<l.points[i];
            }
            os<<"]";
            return os;
        }

        std::string to_string(long long id, bool p3d = false) const {
            std::string result = "Wireframe | " + std::to_string(id) + " | [";
            for (size_t i = 0; i < points.size(); ++i) {
                result += points[i].coords(p3d);
                if (i < points.size() - 1) result += "\n";
            }
            result += "]";
            return result;
        }

        std::tuple<float, float, float> centerPoint() const override {
            float sum_x = 0, sum_y = 0;
            for (const auto &point : points) {
                sum_x += point.x;
                sum_y += point.y;
            }
            return std::make_tuple(sum_x / points.size(), sum_y / points.size());
        }
    };
    
}
#endif // WIREFRAME_H