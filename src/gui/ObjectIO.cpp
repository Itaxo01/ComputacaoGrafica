#include "ObjectIO.hpp"
#include "imgui.h"
#include "ObjectFactories/PointFactory.hpp"
#include "ObjectFactories/WireframeFactory.hpp"
#include "ObjectFactories/PolygonFactory.hpp"
#include "ObjectFactories/Curve2DFactory.hpp"
#include <sstream>

// ─── Validation ──────────────────────────────────────────────────────────────

ObjValidationResult ValidateObjFile(const std::string& path) {
    ObjValidationResult result;
    std::ifstream file(path);
    if (!file.is_open()) { result.error = "Cannot open file: " + path; return result; }

    std::vector<std::tuple<float,float,float>> vertices;
    bool pending_curve2d = false;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line[0] == '#') {
            std::istringstream css(line.substr(1));
            std::string tag; css >> tag;
            if (tag == "color") result.color_count++;
            else if (tag == "type") { std::string t; css >> t; pending_curve2d = (t == "bezier_curve"); }
            continue;
        }
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string type; iss >> type;
        if (type == "v") {
            float x, y, z = 0.0f; iss >> x >> y >> z;
            vertices.emplace_back(x, y, z); result.vertex_count++;
        } else if (type == "o" || type == "g") {
            pending_curve2d = false;
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
            int min_pts = (type == "f") ? 3 : (pending_curve2d ? 4 : 2);
            bool valid_count = (pending_curve2d && type == "l")
                ? (resolved >= 4 && (resolved - 1) % 3 == 0)
                : (resolved >= min_pts);
            if (valid_count) result.object_count++;
        }
    }
    result.valid = true;
    return result;
}

// ─── Export helpers ───────────────────────────────────────────────────────────

static void unpack_color(ImU32 col, int& r, int& g, int& b, int& a) {
    unsigned int uc = (unsigned int)col;
    r = (uc >>  0) & 0xFF;
    g = (uc >>  8) & 0xFF;
    b = (uc >> 16) & 0xFF;
    a = (uc >> 24) & 0xFF;
}

// ─── Export ──────────────────────────────────────────────────────────────────

void ExportObjects(std::ofstream& f, const std::vector<core::Object>& objects,
                   EntityManager& em, int& vi) {
    for (const auto& obj : objects) {
        int r, g, b, a;
        unpack_color(obj.material.color, r, g, b, a);
        f << "o " << obj.name << "\n";
        f << "# color " << r << " " << g << " " << b << " " << a << "\n";

        // World-space vertices: apply obj.transform (mesh stores object-space geometry)
        std::vector<core::Point> wv;
        wv.reserve(obj.mesh->vertices.size());
        for (const auto& v : obj.mesh->vertices) wv.push_back(obj.transform * v);

        switch (obj.type) {
            case core::ObjectType::POINT: {
                if (wv.empty()) break;
                f << "v " << wv[0].x << " " << wv[0].y << " " << wv[0].z << "\n";
                f << "p " << vi << "\n";
                vi++;
                break;
            }
            case core::ObjectType::LINE: {
                if (wv.size() < 2) break;
                f << "v " << wv[0].x << " " << wv[0].y << " " << wv[0].z << "\n";
                f << "v " << wv[1].x << " " << wv[1].y << " " << wv[1].z << "\n";
                f << "l " << vi << " " << vi+1 << "\n";
                vi += 2;
                break;
            }
            case core::ObjectType::WIREFRAME: {
                for (const auto& v : wv)
                    f << "v " << v.x << " " << v.y << " " << v.z << "\n";
                f << "l";
                for (size_t i = 0; i < wv.size(); ++i) f << " " << vi + i;
                f << "\n";
                vi += (int)wv.size();
                break;
            }
            case core::ObjectType::POLYGON: {
                f << "# filled " << (obj.material.filled ? 1 : 0) << "\n";
                // Skip closing duplicate (first == last) if present
                size_t n = wv.size();
                if (n > 1 && wv.front().x == wv.back().x && wv.front().y == wv.back().y) n--;
                for (size_t i = 0; i < n; ++i)
                    f << "v " << wv[i].x << " " << wv[i].y << " " << wv[i].z << "\n";
                f << "f";
                for (size_t i = 0; i < n; ++i) f << " " << vi + i;
                f << "\n";
                vi += (int)n;
                break;
            }
            case core::ObjectType::CURVE2D: {
                f << "# type bezier_curve\n";
                const CurveMetadata* meta = em.getCurveMetadata(obj.id);
                if (meta) {
                    // Export world-space control points
                    for (const auto& p : meta->control_points) {
                        core::Point w = obj.transform * p;
                        f << "v " << w.x << " " << w.y << " " << w.z << "\n";
                    }
                    f << "l";
                    for (size_t i = 0; i < meta->control_points.size(); ++i) f << " " << vi + i;
                    f << "\n";
                    vi += (int)meta->control_points.size();
                }
                break;
            }
            default: break;
        }
    }
}

// ─── Import ──────────────────────────────────────────────────────────────────

static std::vector<core::Point> toPoints(const RawPts& pts) {
    std::vector<core::Point> result;
    result.reserve(pts.size());
    for (const auto& t : pts) result.emplace_back(t);
    return result;
}

void ImportPoint(const std::string& name, const RawPts& pts, int color, EntityManager& em) {
    if (pts.empty()) return;
    auto [x, y, z] = pts[0];
    core::PointFactory f(name, x, y, z, (ImU32)color);
    em.add(f);
}

void ImportWireframe(const std::string& name, const RawPts& pts, int color, EntityManager& em) {
    if (pts.size() < 2) return;
    core::WireframeFactory f(name, toPoints(pts), (ImU32)color);
    em.add(f);
}

void ImportPolygon(const std::string& name, const RawPts& pts, int color, bool filled, EntityManager& em) {
    if (pts.size() < 3) return;
    core::PolygonFactory f(name, toPoints(pts), filled, (ImU32)color);
    em.add(f);
}

void ImportCurve2D(const std::string& name, const RawPts& pts, int color, EntityManager& em) {
    int n = (int)pts.size();
    if (n < 4 || (n - 1) % 3 != 0) return;
    core::Curve2DFactory f(name, toPoints(pts), 50, BEZIER, (ImU32)color);
    em.add(f);
}
