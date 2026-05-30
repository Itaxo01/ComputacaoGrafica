#include "MtlSerializer.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

namespace mtl {

    // Maps a Material's diffuse color (Kd) and opacity (d) onto the flat ImU32
    // color the renderer actually draws with.
    static void refreshRenderColor(core::Material& m) {
        auto to255 = [](float c) {
            int v = (int)(std::clamp(c, 0.0f, 1.0f) * 255.0f + 0.5f);
            return std::clamp(v, 0, 255);
        };
        m.color = IM_COL32(to255(m.diffuse.r), to255(m.diffuse.g),
                           to255(m.diffuse.b), to255(m.dissolve));
    }

    Result Validate(const std::string& path) {
        Result r;
        std::ifstream file(path);
        if (!file.is_open()) { r.error = "Cannot open file: " + path; return r; }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string tag; iss >> tag;
            if (tag == "newmtl") r.material_count++;
        }
        r.ok = true;
        return r;
    }

    Result Load(const std::string& path, MaterialLibrary& lib) {
        Result r;
        std::ifstream file(path);
        if (!file.is_open()) { r.error = "Cannot open file: " + path; return r; }

        core::Material current;
        bool have_current = false;

        auto commit = [&]() {
            if (have_current) {
                refreshRenderColor(current);
                lib[current.name] = current;
                r.material_count++;
            }
        };

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string tag; iss >> tag;
            if (tag.empty() || tag[0] == '#') continue;

            if (tag == "newmtl") {
                commit();
                current = core::Material{};
                have_current = true;
                iss >> current.name;
            } else if (!have_current) {
                continue; // stray parameter before any newmtl — ignore
            } else if (tag == "Ka") {
                iss >> current.ambient.r >> current.ambient.g >> current.ambient.b;
            } else if (tag == "Kd") {
                iss >> current.diffuse.r >> current.diffuse.g >> current.diffuse.b;
            } else if (tag == "Ks") {
                iss >> current.specular.r >> current.specular.g >> current.specular.b;
            } else if (tag == "Ke") {
                iss >> current.emissive.r >> current.emissive.g >> current.emissive.b;
            } else if (tag == "Tf") {
                iss >> current.transmission.r >> current.transmission.g >> current.transmission.b;
            } else if (tag == "Ns") {
                iss >> current.shininess;
            } else if (tag == "Ni") {
                iss >> current.optical_density;
            } else if (tag == "d") {
                iss >> current.dissolve;
            } else if (tag == "Tr") {
                // Transparency is the inverse of dissolve (d = 1 - Tr).
                float tr; if (iss >> tr) current.dissolve = 1.0f - tr;
            } else if (tag == "illum") {
                iss >> current.illum;
            } else if (tag == "map_Ka") {
                iss >> current.map_Ka;
            } else if (tag == "map_Kd") {
                iss >> current.map_Kd;
            } else if (tag == "map_Ks") {
                iss >> current.map_Ks;
            } else if (tag == "map_Ns") {
                iss >> current.map_Ns;
            } else if (tag == "map_d") {
                iss >> current.map_d;
            } else if (tag == "map_Bump" || tag == "bump") {
                iss >> current.map_bump;
            }
            // any other directive is silently skipped
        }
        commit();
        r.ok = true;
        return r;
    }

    // ─── Export ──────────────────────────────────────────────────────────────────

    static bool isZero(const core::Color3& c) { return c.r == 0 && c.g == 0 && c.b == 0; }
    static bool isOne (const core::Color3& c) { return c.r == 1 && c.g == 1 && c.b == 1; }

    Result Export(const std::string& path, const MaterialLibrary& lib) {
        Result r;
        std::ofstream f(path);
        if (!f.is_open()) { r.error = "Cannot open file for writing: " + path; return r; }

        f << std::fixed << std::setprecision(6);
        f << "# Material Count: " << lib.size() << "\n";

        auto color = [&](const char* tag, const core::Color3& c) {
            f << tag << " " << c.r << " " << c.g << " " << c.b << "\n";
        };
        auto map = [&](const char* tag, const std::string& p) {
            if (!p.empty()) f << tag << " " << p << "\n";
        };

        // Sort by name for stable output.
        std::vector<const core::Material*> mats;
        mats.reserve(lib.size());
        for (const auto& [name, m] : lib) mats.push_back(&m);
        std::sort(mats.begin(), mats.end(),
                  [](const core::Material* a, const core::Material* b) { return a->name < b->name; });

        for (const core::Material* mp : mats) {
            const core::Material& m = *mp;
            f << "\nnewmtl " << m.name << "\n";
            f << "Ns " << m.shininess << "\n";
            color("Ka", m.ambient);
            color("Kd", m.diffuse);
            color("Ks", m.specular);
            if (!isZero(m.emissive))      color("Ke", m.emissive);
            if (!isOne(m.transmission))   color("Tf", m.transmission);
            f << "Ni " << m.optical_density << "\n";
            f << "d " << m.dissolve << "\n";
            f << "illum " << m.illum << "\n";
            map("map_Ka", m.map_Ka);
            map("map_Kd", m.map_Kd);
            map("map_Ks", m.map_Ks);
            map("map_Ns", m.map_Ns);
            map("map_d", m.map_d);
            map("map_Bump", m.map_bump);
            r.material_count++;
        }
        r.ok = true;
        return r;
    }
}
