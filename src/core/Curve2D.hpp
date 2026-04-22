#pragma once

#include <ostream>
#include "Point.hpp"
#include "Shape.hpp"
#include <vector>

namespace core{
    class Curve2D: public Shape{
        private:
        core::Point lerp(core::Point p0, core::Point p1, float t) {
            return (p0 + (p1-p0)*t);
        }

        // data format: {P0, C0, C1, P1, C2, C3, P2, ...}
        // anchors at indices 0, 3, 6, 9, ...; controls fill the gaps.
        // each cubic segment uses data[s*3 .. s*3+3]; consecutive segments share endpoints.
        std::vector<core::Point> construct(const std::vector<core::Point> &data) {
            const int smoothness = 25;
            std::vector<core::Point> result;

            int num_segments = ((int)data.size() - 1) / 3;
            for (int seg = 0; seg < num_segments; seg++) {
                core::Point p[4] = {
                    data[seg*3],
                    data[seg*3 + 1],
                    data[seg*3 + 2],
                    data[seg*3 + 3],
                };

                // skip t=0 after the first segment — it's the shared endpoint already added
                int start_i = (seg == 0) ? 0 : 1;
                for (int i = start_i; i < smoothness; i++) {
                    float t = (float)i / (float)(smoothness - 1);
                    core::Point q[4] = {p[0], p[1], p[2], p[3]};
                    for (int d = 3; d > 0; d--)
                        for (int j = 0; j < d; j++)
                            q[j] = lerp(q[j], q[j+1], t);
                    result.push_back(q[0]);
                }
            }
            return result;
        }

        public:
        std::vector<core::Point> points;
        std::vector<core::Point> control_points; // Pontos originais, para exportação e detalhes.

        Curve2D(const std::vector<core::Point> &data) {
            control_points = data;
            points = construct(data);
            type = ShapeType::CURVE2D;
        }

        std::tuple<float, float, float> anchorPoint() const override{
            core::Point p = points.front();
            for(long unsigned int i = 1; i<points.size(); i++){
                p = max_y(p, points[i]);
            }
            return p.expand();
        }


        friend std::ostream &operator<<(std::ostream &os, const Curve2D &l) {
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
            details.type = "Curve 2D";
            details.id = std::to_string(id);
            details.name = this->getName();
            details.color = this->getColor();
            
            std::string pts = "[";
            for (size_t i = 0; i < control_points.size(); ++i) {
                pts += control_points[i].coords(p3d);
                if (i < control_points.size() - 1) pts += "\n";
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