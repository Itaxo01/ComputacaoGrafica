#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include "Point.hpp"
#include "Line.hpp"
#include "Wireframe.hpp"
#include "Polygon.hpp"
#include "Curve2D.hpp"
#include "EntityManager.hpp"

using RawPts = std::vector<std::tuple<float, float, float>>;

// ─── Validation ──────────────────────────────────────────────────────────────

struct ObjValidationResult {
    bool valid = false;
    int  vertex_count  = 0;
    int  object_count  = 0;
    int  color_count   = 0;
    std::string error;
};

ObjValidationResult ValidateObjFile(const std::string &path);

// ─── Export ──────────────────────────────────────────────────────────────────
void ExportPoints      (std::ofstream &f, const std::vector<core::Point>       &v, int &vi);
void ExportLines       (std::ofstream &f, const std::vector<core::Line>        &v, int &vi);
void ExportWireframes  (std::ofstream &f, const std::vector<core::Wireframe>   &v, int &vi);
void ExportPolygons    (std::ofstream &f, const std::vector<core::Polygon>     &v, int &vi);
void ExportCurve2Ds(std::ofstream &f, const std::vector<core::Curve2D> &v, int &vi);

// ─── Import ───────────────────────────────────────────────────────────────────
void ImportPoint      (const std::string &name, const RawPts &pts, int color, EntityManager &em);
void ImportWireframe  (const std::string &name, const RawPts &pts, int color, EntityManager &em);
void ImportPolygon    (const std::string &name, const RawPts &pts, int color, bool filled, EntityManager &em);
void ImportCurve2D(const std::string &name, const RawPts &pts, int color, EntityManager &em);
