#include "ObjectIO.hpp"
#include "ObjSerializer.hpp"
#include "MtlSerializer.hpp"
#include <fstream>
#include <cctype>

namespace {
    bool endsWithCI(const std::string& s, const std::string& suffix) {
        if (s.size() < suffix.size()) return false;
        for (size_t i = 0; i < suffix.size(); ++i)
            if (std::tolower((unsigned char)s[s.size() - suffix.size() + i]) != suffix[i])
                return false;
        return true;
    }
    bool fileExists(const std::string& path) {
        std::ifstream f(path);
        return f.good();
    }
    std::string baseName(const std::string& path) {
        auto slash = path.find_last_of("/\\");
        return (slash == std::string::npos) ? path : path.substr(slash + 1);
    }
}

namespace ObjectIO {

void Import(const std::string& input_path, EntityManager& em, ExampleAppLog& log) {
    std::string obj_path, mtl_path;
    bool mtl_explicit = false;

    if (endsWithCI(input_path, ".obj")) {
        obj_path = input_path;                       // only the .obj
    } else if (endsWithCI(input_path, ".mtl")) {
        mtl_path = input_path; mtl_explicit = true;  // only the .mtl
    } else {
        obj_path = input_path + ".obj";              // pair both by base name
        mtl_path = input_path + ".mtl";              // companion is optional
    }

    mtl::MaterialLibrary lib;

    // 1) Load the companion / explicit .mtl up front so `usemtl` can resolve.
    if (!mtl_path.empty() && (mtl_explicit || fileExists(mtl_path))) {
        mtl::Result mr = mtl::Load(mtl_path, lib);
        if (mr.ok)
            log.AddLog("[info] Loaded %d material(s) from %s\n", mr.material_count, mtl_path.c_str());
        else
            log.AddLog("[error] %s\n", mr.error.c_str());
    }

    if (obj_path.empty()) {
        log.AddLog("[info] No geometry imported (materials only).\n");
        return;
    }

    // 2) Validate geometry before touching the scene.
    obj::ValidationResult v = obj::Validate(obj_path);
    if (!v.valid) { log.AddLog("[error] %s\n", v.error.c_str()); return; }
    log.AddLog("[info] Validation passed: %d vertices, %d objects, %d colored.\n",
               v.vertex_count, v.object_count, v.color_count);

    // 3) Import geometry. Any `mtllib` directives merge into `lib` during this pass.
    obj::ImportResult r = obj::Import(obj_path, lib, em);
    if (!r.error.empty()) { log.AddLog("[error] %s\n", r.error.c_str()); return; }
    for (const auto& w : r.warnings) log.AddLog("[warn] %s\n", w.c_str());
    log.AddLog("Imported %d objects from %s\n", r.object_count, obj_path.c_str());
}

void Export(const std::string& input_path, EntityManager& em, ExampleAppLog& log) {
    std::string path = input_path;
    if (endsWithCI(path, ".mtl")) path = path.substr(0, path.size() - 4);
    if (!endsWithCI(path, ".obj")) path += ".obj";
    const std::string mtl_path = path.substr(0, path.size() - 4) + ".mtl";

    // Gather the named materials carried by mesh objects into a library.
    mtl::MaterialLibrary lib;
    for (const auto& obj : em.getObjects())
        for (const auto& m : obj.mesh->materials)
            if (!m.name.empty()) lib[m.name] = m;

    std::ofstream file(path);
    if (!file.is_open()) { log.AddLog("[error] Failed to open file: %s\n", path.c_str()); return; }

    int vi = 1;
    file << "# Exported by ComputacaoGrafica\n";
    if (!lib.empty()) file << "mtllib " << baseName(mtl_path) << "\n";
    obj::Export(file, em.getObjects(), em, vi);
    file.close();
    log.AddLog("Exported objects to %s\n", path.c_str());

    // Companion .mtl for the materials referenced above.
    if (!lib.empty()) {
        mtl::Result mr = mtl::Export(mtl_path, lib);
        if (mr.ok) log.AddLog("[info] Wrote %d material(s) to %s\n", mr.material_count, mtl_path.c_str());
        else       log.AddLog("[error] %s\n", mr.error.c_str());
    }
}

}
