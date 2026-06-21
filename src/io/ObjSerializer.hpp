#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Object.hpp"
#include "EntityManager.hpp"
#include "MtlSerializer.hpp"

// Serializer for Wavefront .obj geometry files.
//
// Handles the deserialization (Validate/Import) and serialization (Export) of
// objects. Material references are resolved against a mtl::MaterialLibrary:
// `usemtl <name>` selects which material the following geometry carries, and
// any `mtllib <file>` directive is loaded into the library on the fly.
namespace obj {

    struct ValidationResult {
        bool valid = false;
        int  vertex_count = 0;
        int  object_count = 0;
        int  color_count  = 0;
        std::string error;
    };

    // Structural check without mutating any state.
    ValidationResult Validate(const std::string& path);

    struct ImportResult {
        int object_count = 0;
        std::vector<std::string> warnings;   // non-fatal issues (e.g. unknown usemtl)
        std::string error;                   // non-empty only on hard failure
    };

    // Imports geometry from `path`, creating objects in `em`. `lib` provides
    // materials referenced by `usemtl`; `mtllib` directives found in the file
    // are additionally loaded into `lib` (resolved relative to the .obj's dir).
    ImportResult Import(const std::string& path, mtl::MaterialLibrary& lib, EntityManager& em);

    // Writes all objects to an already-open stream, advancing the running 1-based
    // vertex index `vi` (OBJ convention).
    void Export(std::ofstream& f, const std::vector<core::Object>& objects,
                EntityManager& em, int& vi);
}
