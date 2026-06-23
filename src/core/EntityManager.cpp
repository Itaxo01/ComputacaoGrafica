#include "EntityManager.hpp"
#include "ObjectFactories/PointFactory.hpp"
#include "ObjectFactories/LineFactory.hpp"
#include "ObjectFactories/WireframeFactory.hpp"
#include "ObjectFactories/PolygonFactory.hpp"
#include "ObjectFactories/Curve2DFactory.hpp"
#include "ObjectFactories/SurfaceFactory.hpp"

static std::string colorStr(ImU32 col) {
    unsigned int uc = (unsigned int)col;
    int r = (uc >> 0)  & 0xFF;
    int g = (uc >> 8)  & 0xFF;
    int b = (uc >> 16) & 0xFF;
    return "[" + std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + "]";
}

long long EntityManager::add(core::ObjectFactory& factory) {
    long long id = nextID();
    core::Object obj = factory.build();
    if (obj.name.empty())
        obj.name = autoName(obj.type, id);
    obj.id = id;
    displayFile.add(std::move(obj));
    auto meta = factory.takeMetadata();
    if (meta) displayFile.setMetadata(id, std::move(meta));
    renderer.notifyTransformation();
    return id;
}

core::ObjectDetails EntityManager::GetObjectDetails(long long id) const {
    if (displayFile.getHashID().find(id) == displayFile.getHashID().end())
        return core::ObjectDetails{"Not found", "", "", "", ""};

    const core::Object& obj = displayFile.getObject(id);
    core::ObjectDetails d;
    d.type  = core::getTypeName(obj.type);
    d.id    = std::to_string(id);
    d.name  = obj.name;
    d.color = colorStr(obj.material.color);

    if (obj.type == core::ObjectType::CURVE2D) {
        const CurveMetadata* meta = displayFile.getMetadata<CurveMetadata>(id);
        if (meta) {
            std::string pts = "[";
            for (size_t i = 0; i < meta->control_points.size(); ++i) {
                pts += (obj.transform * meta->control_points[i]).coords();
                if (i + 1 < meta->control_points.size()) pts += "\n";
            }
            pts += "]";
            d.points = pts;
            return d;
        }
    }
    if (obj.type == core::ObjectType::SURFACE) {
        const SurfaceMetadata* meta = displayFile.getMetadata<SurfaceMetadata>(id);
        if (meta) {
            std::string pts = "[";
            for (size_t i = 0; i < meta->control_points.size(); ++i) {
                pts += (obj.transform * meta->control_points[i]).coords();
                if (i + 1 < meta->control_points.size()) pts += "\n";
            }
            pts += "]";
            d.points = pts;
            return d;
        }
    }

    // All other types: use mesh vertices
    std::string pts = "[";
    for (size_t i = 0; i < obj.mesh->vertices.size(); ++i) {
        pts += obj.mesh->vertices[i].coords();
        if (i + 1 < obj.mesh->vertices.size()) pts += "\n";
    }
    pts += "]";
    if (obj.type == core::ObjectType::POLYGON)
        pts += obj.material.filled ? " (filled)" : " (outline)";
    d.points = pts;
    return d;
}

std::vector<std::tuple<float,float,float>> EntityManager::GetObjectRawPoints(long long id) const {
    if (displayFile.getHashID().find(id) == displayFile.getHashID().end()) return {};
    const core::Object& obj = displayFile.getObject(id);

    if (obj.type == core::ObjectType::CURVE2D) {
        const CurveMetadata* meta = displayFile.getMetadata<CurveMetadata>(id);
        if (meta) {
            std::vector<std::tuple<float,float,float>> result;
            for (const auto& p : meta->control_points) {
                core::Point w = obj.transform * p;
                result.emplace_back(w.x, w.y, w.z);
            }
            return result;
        }
    }

    if (obj.type == core::ObjectType::SURFACE) {
        const SurfaceMetadata* meta = displayFile.getMetadata<SurfaceMetadata>(id);
        if (meta) {
            std::vector<std::tuple<float,float,float>> result;
            for (const auto& cp : meta->control_points) {
                core::Point w = obj.transform * cp;
                result.emplace_back(w.x, w.y, w.z);
            }
            return result;
        }
    }
    return obj.GetRawPoints();
}

void EntityManager::UpdateObjectPoints(long long id,
                                        const std::vector<std::tuple<float,float,float>>& new_pts,
                                        core::ObjectType new_type, int new_method, bool new_filled,
                                        int new_rows, int new_cols) {
    if (displayFile.getHashID().find(id) == displayFile.getHashID().end()) return;

    const core::Object& old = displayFile.getObject(id);
    std::string name  = old.name;
    ImU32       color = old.material.color;
    int smoothness = 50;
    const CurveMetadata* meta = displayFile.getMetadata<CurveMetadata>(id);
    if (meta) smoothness = meta->smoothness;

    int surf_resolution = 12;
    int surf_technique  = SURF_FORWARD_DIFF;
    const SurfaceMetadata* smeta = displayFile.getMetadata<SurfaceMetadata>(id);
    if (smeta) { surf_resolution = smeta->resolution; surf_technique = smeta->technique; }

    displayFile.remove(id);

    std::vector<core::Point> pts;
    pts.reserve(new_pts.size());
    for (const auto& t : new_pts) pts.emplace_back(t);

    switch (new_type) {
        case core::ObjectType::POINT: {
            core::PointFactory f(name, pts[0].x, pts[0].y, pts[0].z, color);
            add(f); break;
        }
        case core::ObjectType::LINE: {
            core::LineFactory f(name, pts[0], pts[1], color);
            add(f); break;
        }
        case core::ObjectType::WIREFRAME: {
            core::WireframeFactory f(name, pts, color);
            add(f); break;
        }
        case core::ObjectType::POLYGON: {
            core::PolygonFactory f(name, pts, new_filled, color);
            add(f); break;
        }
        case core::ObjectType::CURVE2D: {
            core::Curve2DFactory f(name, pts, smoothness, new_method, color);
            add(f); break;
        }
        case core::ObjectType::SURFACE: {
            if (new_rows >= 4 && new_cols >= 4 && (int)pts.size() >= new_rows * new_cols) {
                core::SurfaceFactory f(name, new_rows, new_cols, pts, new_method, surf_technique, surf_resolution, color);
                add(f);
            }
            break;
        }
        default: break;
    }
}

void EntityManager::ApplyTransformation(long long id, const core::mat4& matrix) {
    displayFile.getObject(id).ApplyTransformation(matrix);
    renderer.notifyTransformation();
}

void EntityManager::SetTransformation(long long id, const core::mat4& matrix) {
    displayFile.getObject(id).transform = matrix;
    renderer.notifyTransformation();
}
