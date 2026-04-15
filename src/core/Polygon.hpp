#ifndef POLYGON_H
#define POLYGON_H

#include <algorithm>
#include <ostream>
#include <vector>
#include "Point.hpp"
#include "Shape.hpp"

namespace core {

    /*
        Polygon: closed shape defined by an ordered list of vertices.
        The last vertex is implicitly connected back to the first — callers
        should NOT duplicate the first vertex at the end.

        filled = false → render as a closed outline (same clipping as Wireframe)
        filled = true  → render as a filled polygon (Sutherland-Hodgman clipping
                          + triangle-fan triangulation, to be implemented)
    */
    class Polygon : public Shape {
    public:
        std::vector<core::Point> points;
        bool filled = false;

        Polygon(std::vector<core::Point> &data, bool filled = false)
            : points(data), filled(filled)
        {
            type = ShapeType::POLYGON;
        }

        std::tuple<float, float, float> anchorPoint() const override {
            core::Point p = points.front();
            for (size_t i = 1; i < points.size(); i++)
                p = max_y(p, points[i]);
            return p.expand();
        }

        std::tuple<float, float, float> centerPoint() const override {
            float sx = 0, sy = 0, sz = 0;
            for (const auto &p : points) { sx += p.x; sy += p.y; sz += p.z; }
            float n = (float)points.size();
            return std::make_tuple(sx / n, sy / n, sz / n);
        }

        ObjectDetails GetObjectDetails(long long id, bool p3d = false) const {
            ObjectDetails details;
            details.type = "Polygon";
            details.id = std::to_string(id);
            details.name = this->getName();
            details.color = this->getColor();
            
            std::string pts = "[";
            for (size_t i = 0; i < points.size(); ++i) {
                pts += points[i].coords(p3d);
                if (i < points.size() - 1) pts += "\n";
            }
            pts += "]";
            pts += filled ? " (filled)" : " (outline)";
            details.points = pts;
            
            return details;
        }

        friend std::ostream &operator<<(std::ostream &os, const Polygon &pg) {
            os << "[";
            for (size_t i = 0; i < pg.points.size(); i++) {
                if (i) os << " - ";
                os << pg.points[i];
            }
            os << "]";
            return os;
        }
    };

} // namespace core

#endif // POLYGON_H
