#include "ObjectIO.hpp"
#include "imgui.h"
#include <sstream>

// ─── Validation ──────────────────────────────────────────────────────────────

ObjValidationResult ValidateObjFile(const std::string &path) {
    ObjValidationResult result;

    std::ifstream file(path);
    if (!file.is_open()) {
        result.error = "Cannot open file: " + path;
        return result;
    }

    std::vector<std::tuple<float,float,float>> vertices;
    bool pending_bezier = false;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line[0] == '#') {
            std::istringstream css(line.substr(1));
            std::string tag; css >> tag;
            if (tag == "color") result.color_count++;
            else if (tag == "type") {
                std::string t; css >> t;
                pending_bezier = (t == "bezier_curve");
            }
            continue;
        }
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string type; iss >> type;

        if (type == "v") {
            float x, y, z = 0.0f; iss >> x >> y >> z;
            vertices.emplace_back(x, y, z);
            result.vertex_count++;
        } else if (type == "o" || type == "g") {
            pending_bezier = false;
        } else if (type == "p") {
            std::string v_str;
            while (iss >> v_str) {
                try {
                    int idx = std::stoi(v_str);
                    if (idx < 0) idx = (int)vertices.size() + idx + 1;
                    if (idx > 0 && idx <= (int)vertices.size()) { result.object_count++; break; }
                } catch (...) {}
            }
        } else if (type == "l" || type == "f") {
            int resolved = 0;
            std::string v_str;
            while (iss >> v_str) {
                try {
                    int idx = std::stoi(v_str);
                    if (idx < 0) idx = (int)vertices.size() + idx + 1;
                    if (idx > 0 && idx <= (int)vertices.size()) resolved++;
                } catch (...) {}
            }
            int min_pts = (type == "f") ? 3 : (pending_bezier ? 4 : 2);
            bool valid_count = (pending_bezier && type == "l")
                ? (resolved >= 4 && (resolved - 1) % 3 == 0)
                : (resolved >= min_pts);
            if (valid_count) result.object_count++;
        }
    }

    result.valid = true;
    return result;
}

static void unpack_color(int col, int &r, int &g, int &b, int &a) {
    unsigned int uc = (unsigned int)col;
    r = (uc >>  0) & 0xFF;
    g = (uc >>  8) & 0xFF;
    b = (uc >> 16) & 0xFF;
    a = (uc >> 24) & 0xFF;
}

// ─── Export ──────────────────────────────────────────────────────────────────

void ExportPoints(std::ofstream &f, const std::vector<core::Point> &v, int &vi) {
    for (const auto &pt : v) {
        int r, g, b, a;
        unpack_color(pt.object_color, r, g, b, a);
        f << "o " << pt.getName() << "\n";
        f << "# color " << r << " " << g << " " << b << " " << a << "\n";
        f << "v " << pt.x << " " << pt.y << " 0.0\n";
        f << "p " << vi << "\n";
        vi++;
    }
}

void ExportLines(std::ofstream &f, const std::vector<core::Line> &v, int &vi) {
    for (const auto &ln : v) {
        int r, g, b, a;
        unpack_color(ln.object_color, r, g, b, a);
        f << "o " << ln.getName() << "\n";
        f << "# color " << r << " " << g << " " << b << " " << a << "\n";
        f << "v " << ln.a.x << " " << ln.a.y << " 0.0\n";
        f << "v " << ln.b.x << " " << ln.b.y << " 0.0\n";
        f << "l " << vi << " " << vi + 1 << "\n";
        vi += 2;
    }
}

void ExportWireframes(std::ofstream &f, const std::vector<core::Wireframe> &v, int &vi) {
    for (const auto &w : v) {
        int r, g, b, a;
        unpack_color(w.object_color, r, g, b, a);
        f << "o " << w.getName() << "\n";
        f << "# color " << r << " " << g << " " << b << " " << a << "\n";
        for (const auto &p : w.points)
            f << "v " << p.x << " " << p.y << " 0.0\n";
        f << "l";
        for (size_t i = 0; i < w.points.size(); ++i)
            f << " " << vi + i;
        f << "\n";
        vi += (int)w.points.size();
    }
}

void ExportPolygons(std::ofstream &f, const std::vector<core::Polygon> &v, int &vi) {
    for (const auto &poly : v) {
        int r, g, b, a;
        unpack_color(poly.object_color, r, g, b, a);
        f << "o " << poly.getName() << "\n";
        f << "# color " << r << " " << g << " " << b << " " << a << "\n";
        f << "# filled " << (poly.filled ? 1 : 0) << "\n";
        // polygons store a closing duplicate of the first vertex — skip it on export
        size_t n = poly.points.size();
        if (n > 1 && poly.points.front().x == poly.points.back().x &&
                     poly.points.front().y == poly.points.back().y)
            n--;
        for (size_t i = 0; i < n; ++i)
            f << "v " << poly.points[i].x << " " << poly.points[i].y << " 0.0\n";
        f << "f";
        for (size_t i = 0; i < n; ++i)
            f << " " << vi + i;
        f << "\n";
        vi += (int)n;
    }
}

void ExportBezierCurves(std::ofstream &f, const std::vector<core::BezierCurve> &v, int &vi) {
    for (const auto &bc : v) {
        int r, g, b, a;
        unpack_color(bc.object_color, r, g, b, a);
        f << "o " << bc.getName() << "\n";
        f << "# color " << r << " " << g << " " << b << " " << a << "\n";
        f << "# type bezier_curve\n";
        for (const auto &p : bc.control_points)
            f << "v " << p.x << " " << p.y << " 0.0\n";
        f << "l";
        for (size_t i = 0; i < bc.control_points.size(); ++i)
            f << " " << vi + i;
        f << "\n";
        vi += (int)bc.control_points.size();
    }
}

// ─── Import ──────────────────────────────────────────────────────────────────

void ImportPoint(const std::string &name, const RawPts &pts, int color, EntityManager &em) {
    if (pts.empty()) return;
    auto mode = core::ShapeType::POINT;
    RawPts single = {pts[0]};
    em.add(name, single, mode, false, color);
}

void ImportWireframe(const std::string &name, const RawPts &pts, int color, EntityManager &em) {
    if (pts.size() < 2) return;
    auto mode = core::ShapeType::WIREFRAME;
    RawPts copy = pts;
    em.add(name, copy, mode, false, color);
}

void ImportPolygon(const std::string &name, const RawPts &pts, int color, bool filled, EntityManager &em) {
    if (pts.size() < 3) return;
    auto mode = core::ShapeType::POLYGON;
    RawPts copy = pts;
    em.add(name, copy, mode, filled, color);
}

void ImportBezierCurve(const std::string &name, const RawPts &pts, int color, EntityManager &em) {
    int n = (int)pts.size();
    if (n < 4 || (n - 1) % 3 != 0) return;
    auto mode = core::ShapeType::BEZIER_CURVE;
    RawPts copy = pts;
    em.add(name, copy, mode, false, color);
}
