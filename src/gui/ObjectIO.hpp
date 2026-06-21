#pragma once
#include <string>
#include "EntityManager.hpp"
#include "log_app.h"

// High-level scene import/export facade. Resolves the user-supplied path and
// delegates the actual file parsing/writing to the specialized serializers
// (see src/io/ObjSerializer and src/io/MtlSerializer); this layer only owns the
// filename-pairing policy and user-facing logging.
//
// Base-name pairing rule:
//   "x"      -> imports x.obj and, if present, its companion x.mtl
//   "x.obj"  -> imports only x.obj (a `mtllib` referenced inside is still honored)
//   "x.mtl"  -> loads only x.mtl (materials only; no geometry is created)
namespace ObjectIO {
    void Import(const std::string& input_path, EntityManager& em, ExampleAppLog& log);
    void Export(const std::string& input_path, EntityManager& em, ExampleAppLog& log);
}
