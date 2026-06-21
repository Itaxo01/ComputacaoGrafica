#pragma once
#include <string>
#include <unordered_map>
#include "Material.hpp"

// Serializer for Wavefront .mtl material libraries.
//
// A .mtl file is just a named library of materials; it has NO knowledge of the
// objects or .obj files that reference it. The link is established by the .obj
// side via `mtllib <file>` (which library to load) and `usemtl <name>` (which
// material a group of faces uses). This serializer only turns the text into an
// in-memory library keyed by material name — tying materials to objects is the
// importer's job (see ObjSerializer).
namespace mtl {

    using MaterialLibrary = std::unordered_map<std::string, core::Material>;

    struct Result {
        bool        ok = false;
        int         material_count = 0;
        std::string error;   // non-empty only on hard failure (e.g. cannot open)
    };

    // Quick structural check without mutating any state.
    Result Validate(const std::string& path);

    // Parse `path` and insert/overwrite each material into `lib`.
    // Materials already present under the same name are replaced.
    Result Load(const std::string& path, MaterialLibrary& lib);

    // Writes every material in `lib` to `path` in .mtl form (semantic fidelity:
    // re-loadable and lossless in meaning; float formatting is canonicalized).
    Result Export(const std::string& path, const MaterialLibrary& lib);
}
