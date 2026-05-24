#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include "Object.hpp"
#include "ObjectMetadatas/CurveMetadata.hpp"
#include "EntityManager.hpp"

using RawPts = std::vector<std::tuple<float, float, float>>;

// ─── Validation ──────────────────────────────────────────────────────────────

struct ObjValidationResult {
    bool valid = false;
    int  vertex_count = 0;
    int  object_count = 0;
    int  color_count  = 0;
    std::string error;
};

ObjValidationResult ValidateObjFile(const std::string& path);

// ─── Export ──────────────────────────────────────────────────────────────────

void ExportObjects(std::ofstream& f, const std::vector<core::Object>& objects,
                   EntityManager& em, int& vi);

// ─── Import ──────────────────────────────────────────────────────────────────

void ImportPoint    (const std::string& name, const RawPts& pts, int color, EntityManager& em);
void ImportWireframe(const std::string& name, const RawPts& pts, int color, EntityManager& em);
void ImportPolygon  (const std::string& name, const RawPts& pts, int color, bool filled, EntityManager& em);
void ImportCurve2D  (const std::string& name, const RawPts& pts, int color, EntityManager& em);
