#pragma once

#include <ostream>
#include "Point.hpp"
#include "Shape.hpp"
#include <vector>

#define BEZIER 0
#define BSPLINE 1

namespace core{
    class Curve2D: public Shape{
        private:
        core::Point lerp(core::Point p0, core::Point p1, float t) {
            return (p0 + (p1-p0)*t);
        }

        // data format: {P0, C0, C1, P1, C2, C3, P2, ...}
        // anchors at indices 0, 3, 6, 9, ...; controls fill the gaps.
        // each cubic segment uses data[s*3 .. s*3+3]; consecutive segments share endpoints.
        inline std::vector<core::Point> construct_bezier(const std::vector<core::Point> &data, int steps) {
            std::vector<core::Point> result;
            // steps = smoothness
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
                for (int i = start_i; i < steps; i++) {
                    float t = (float)i / (float)(steps - 1);
                    core::Point q[4] = {p[0], p[1], p[2], p[3]};
                    for (int d = 3; d > 0; d--)
                        for (int j = 0; j < d; j++)
                            q[j] = lerp(q[j], q[j+1], t);
                    result.push_back(q[0]);
                }
            }
            return result;
        }

        inline std::vector<core::Point> construct_bspline(const std::vector<core::Point> &data, int steps) {
            std::vector<core::Point> result;
            
            // A B-spline requires at least 4 control points to form a single cubic segment
            if (data.size() < 4) return result;

            // B-Splines use a sliding window, so the number of segments is N - 3
            int num_segments = data.size() - 3;
            
            // delta is the parametric step size (t goes from 0 to 1)
            float delta = 1.0f / (float)(steps - 1);
            float d2 = delta * delta;
            float d3 = d2 * delta;

            for (int seg = 0; seg < num_segments; seg++) {
                // 1. Sliding window of 4 consecutive points
                core::Point p0 = data[seg];
                core::Point p1 = data[seg + 1];
                core::Point p2 = data[seg + 2];
                core::Point p3 = data[seg + 3];

                // 2. Calculate polynomial coefficients (A, B, C, D) using the B-Spline Basis Matrix
                // Note: The 1/6 scalar is standard to keep the curve within the convex hull
                core::Point A = (p0 * -1.0f + p1 *  3.0f + p2 * -3.0f + p3 * 1.0f) * (1.0f / 6.0f);
                core::Point B = (p0 *  3.0f + p1 * -6.0f + p2 *  3.0f + p3 * 0.0f) * (1.0f / 6.0f);
                core::Point C = (p0 * -3.0f + p1 *  0.0f + p2 *  3.0f + p3 * 0.0f) * (1.0f / 6.0f);
                core::Point D = (p0 *  1.0f + p1 *  4.0f + p2 *  1.0f + p3 * 0.0f) * (1.0f / 6.0f);

                // 3. Initialize the Forward Differences for t = 0
                core::Point f   = D;
                core::Point df  = A * d3 + B * d2 + C * delta;
                core::Point d2f = A * (6.0f * d3) + B * (2.0f * d2);
                core::Point d3f = A * (6.0f * d3);

                // 4. The Iterative Addition Loop
                for (int i = 0; i < steps; i++) {
                    // B-splines have C2 continuity, so the end of segment N is exactly the start of segment N+1
                    // We skip pushing i=0 for segments after the first to avoid duplicate points
                    if (seg == 0 || i > 0) {
                        result.push_back(f);
                    }
                    
                    // Advance to the next coordinate using only additions
                    f   = f + df;
                    df  = df + d2f;
                    d2f = d2f + d3f;
                }
            }
            return result;
        }

        public:
        int smoothness;
        std::vector<core::Point> points;
        std::vector<core::Point> control_points; // Pontos originais, para exportação e detalhes.

        Curve2D(const std::vector<core::Point> &data, int smoothness = 50, int method = BEZIER) : smoothness(smoothness) {
            control_points = data;
            switch(method) {
                case BEZIER:
                    points = construct_bezier(data, smoothness);
                    break;
                case BSPLINE:
                    points = construct_bspline(data, smoothness);
                    break;
                default:
                    points = construct_bezier(data, smoothness);
                }
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

        ObjectDetails GetObjectDetails(long long id) const {
            ObjectDetails details;
            details.type = "Curve 2D";
            details.id = std::to_string(id);
            details.name = this->getName();
            details.color = this->getColor();

            std::string pts = "[";
            for (size_t i = 0; i < control_points.size(); ++i) {
                pts += control_points[i].coords();
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