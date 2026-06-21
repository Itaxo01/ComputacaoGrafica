#pragma once
#include <vector>
#include <utility>
#include <tuple>
#include <string>
#include <cstdint>
#include "Point.hpp"
#include "Material.hpp"

namespace core {

    // Texture coordinate (OBJ `vt`). v/w default to 0 for 1-D / 2-D coords.
    struct TexCoord {
        float u = 0.0f, v = 0.0f, w = 0.0f;
    };

    // One corner of a face. Indices are 0-based into the owning Mesh's arrays;
    // -1 means "absent" (OBJ allows v, v/vt, v//vn, v/vt/vn forms).
    struct FaceVertex {
        int v  = -1;   // position index  -> Mesh::vertices
        int vt = -1;   // texcoord index  -> Mesh::texcoords
        int vn = -1;   // normal index    -> Mesh::normals
    };

    // A polygonal face (triangle, quad or n-gon), tagged with the state that was
    // active when it was declared so it can be re-emitted exactly on export.
    struct Face {
        std::vector<FaceVertex> corners;
        int material  = -1;   // index into Mesh::materials (-1 = none / default)
        int group     = -1;   // index into Mesh::groups    (-1 = none)
        int smoothing = 0;    // OBJ `s` smoothing group (0 = off)
    };

    struct Mesh {
        // ── Geometry used by the current (wireframe/point) renderer ─────────────
        std::vector<core::Point> vertices;                              // OBJ `v`
        std::vector<std::pair<uint32_t, uint32_t>> line_indices;        // drawn edges
        std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> tri_indices; // filled tris

        // ── Full-fidelity OBJ data (parsed & exported, not yet rendered) ────────
        std::vector<TexCoord>     texcoords;   // OBJ `vt`            (not rendered: no texturing)
        std::vector<core::Point>  normals;     // OBJ `vn`            (not rendered: no lighting)
        std::vector<Face>         faces;       // OBJ `f` w/ v/vt/vn  (rendered only as wireframe edges)
        std::vector<std::string>  groups;      // OBJ `g` names referenced by faces
        std::vector<Material>     materials;   // materials referenced by faces (full .mtl params)
    };
}
