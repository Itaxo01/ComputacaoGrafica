#ifndef BEZIER_CURVE_H
#define BEZIER_CURVE_H

#include <ostream>
#include "Point.hpp"
#include "Shape.hpp"
#include <vector>

namespace core{
    class BezierCurve: public Shape{
        private:
        // depois colocar em outro lugar
        core::Point lerp(core::Point p0, core::Point p1, float t) {
            return (p0 + (p1-p0)*t);
        }

        // data arrives in the format: {p0, p1, c0 ,c1, c2, ... cn}
        std::vector<core::Point> construct(std::vector<core::Point> &data) {
            // changes data to the format: {p0, c0 ,c1, c2, ... cn, p1}
            core::Point last_point_buffer = data[1];
            data.erase(next(data.begin()));
            data.push_back(data[1]);

            const int smoothness = 25; // number of evaluated points. number of lines will be smoothness - 1.
            const float delta_t = 1.0f / (float) smoothness;

            // memory allocation
            std::vector<core::Point> points(smoothness);
            std::vector<std::vector<core::Point>> control_buffer;
            for (int vector_size = data.size(); vector_size > 0; vector_size--)
                control_buffer.push_back(std::vector<core::Point>(vector_size));
            control_buffer[0] = data;

            // points evaluation
            int counter = 0;
            for (float t = 0.0f; t < 1.0f; t += delta_t, counter++) {
                for (int buffer_depth = 0; buffer_depth < (int) control_buffer.size(); buffer_depth++) {
                    for (int i = 0; i < (int) control_buffer[buffer_depth].size()-1; i++) {
                        control_buffer[buffer_depth+1][i] = lerp(control_buffer[buffer_depth][i], control_buffer[buffer_depth][i+1], t);
                    }
                }
                points[counter] = control_buffer[control_buffer.size()-1][0];
            }
            return points;
        }

        public:
        std::vector<core::Point> points;
        BezierCurve(std::vector<core::Point> &data) {
            points = construct(data);
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