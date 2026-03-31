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

        std::pair<float, float> anchorPoint() const {
            core::Point p = points.front();
            for(long unsigned int i = 1; i<points.size(); i++){
                p = max_y(p, points[i]);
            }
            return std::make_pair(p.x, p.y);
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

        std::string to_string(long long id) const {
            std::string result = "Wireframe | " + std::to_string(id) + " | [";
            for (size_t i = 0; i < points.size(); ++i) {
                result += "(" + format(points[i].x, 2) + ", " + format(points[i].y, 2) + ")";
                if (i < points.size() - 1) result += "\n";
            }
            result += "]";
            return result;
        }
    };
    
}
#endif // WIREFRAME_H