#include "ObjSerializer.hpp"
#include "ObjectFactories/PointFactory.hpp"
#include "ObjectFactories/WireframeFactory.hpp"
#include "ObjectFactories/PolygonFactory.hpp"
#include "ObjectFactories/Curve2DFactory.hpp"
#include "ObjectFactories/MeshFactory.hpp"
#include "ObjectMetadatas/CurveMetadata.hpp"
#include <sstream>
#include <unordered_map>
#include <climits>

namespace obj {

    using RawPts = std::vector<std::tuple<float, float, float>>;

    // ─── Validation ────────────────────────────────────────────────────────────

    ValidationResult Validate(const std::string& path) {
        ValidationResult result;
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

    // ─── Primitive import helpers (our own #-comment format) ─────────────────────

    static std::vector<core::Point> toPoints(const RawPts& pts) {
        std::vector<core::Point> result;
        result.reserve(pts.size());
        for (const auto& t : pts) result.emplace_back(t);
        return result;
    }

    static void importPoint(const std::string& name, const RawPts& pts, int color, EntityManager& em) {
        if (pts.empty()) return;
        auto [x, y, z] = pts[0];
        core::PointFactory f(name, x, y, z, (ImU32)color);
        em.add(f);
    }
    static void importWireframe(const std::string& name, const RawPts& pts, int color, EntityManager& em) {
        if (pts.size() < 2) return;
        core::WireframeFactory f(name, toPoints(pts), (ImU32)color);
        em.add(f);
    }
    static void importPolygon(const std::string& name, const RawPts& pts, int color, bool filled, EntityManager& em) {
        if (pts.size() < 3) return;
        core::PolygonFactory f(name, toPoints(pts), filled, (ImU32)color);
        em.add(f);
    }
    static void importCurve2D(const std::string& name, const RawPts& pts, int color, EntityManager& em) {
        int n = (int)pts.size();
        if (n < 4 || (n - 1) % 3 != 0) return;
        core::Curve2DFactory f(name, toPoints(pts), 50, BEZIER, (ImU32)color);
        em.add(f);
    }

    static std::string dirOf(const std::string& path) {
        auto slash = path.find_last_of("/\\");
        return (slash == std::string::npos) ? std::string() : path.substr(0, slash + 1);
    }

    // Resolves `usemtl <name>` against the library; an unknown/blank name yields a
    // default white material so geometry still gets a sensible flat color.
    static core::Material resolveMaterial(const std::string& name, const mtl::MaterialLibrary& lib) {
        auto it = lib.find(name);
        if (it != lib.end()) return it->second;
        core::Material m;
        m.name  = name;
        m.color = IM_COL32_WHITE;
        return m;
    }

    // Parses one face token ("v", "v/vt", "v//vn", "v/vt/vn") into 0-based indices,
    // resolving OBJ's 1-based and negative (relative) forms against current counts.
    static core::FaceVertex parseCorner(const std::string& tok, int nPos, int nUv, int nNorm) {
        core::FaceVertex fv;
        int vals[3] = {0, 0, 0};
        bool has[3] = {false, false, false};
        int part = 0; size_t start = 0;
        for (size_t i = 0; i <= tok.size() && part < 3; ++i) {
            if (i == tok.size() || tok[i] == '/') {
                std::string s = tok.substr(start, i - start);
                if (!s.empty()) { try { vals[part] = std::stoi(s); has[part] = true; } catch (...) {} }
                start = i + 1; part++;
            }
        }
        auto resolve = [](int v, int n) -> int {
            if (v > 0) return v - 1;
            if (v < 0) return n + v;   // -1 == last
            return -1;
        };
        if (has[0]) fv.v  = resolve(vals[0], nPos);
        if (has[1]) fv.vt = resolve(vals[1], nUv);
        if (has[2]) fv.vn = resolve(vals[2], nNorm);
        return fv;
    }

    // ─── Import ────────────────────────────────────────────────────────────────

    namespace {
        struct PendingFace {
            std::vector<core::FaceVertex> corners;  // global 0-based indices
            std::string group;
            int smoothing = 0;
        };
    }

    ImportResult Import(const std::string& path, mtl::MaterialLibrary& lib, EntityManager& em) {
        ImportResult res;
        std::ifstream file(path);
        if (!file.is_open()) { res.error = "Failed to open file: " + path; return res; }

        const std::string base_dir = dirOf(path);

        // File-global geometry pools (OBJ indices reference these globally).
        std::vector<core::Point>    positions;
        std::vector<core::TexCoord> uvs;
        std::vector<core::Point>    normals;

        // Current parser state.
        std::string current_object;
        std::string current_group;
        std::string current_material;
        int  current_smoothing = 0;
        int  pending_color = IM_COL32_WHITE;  // from our "# color"
        bool pending_filled = false;          // from our "# filled" (marks our polygons)
        bool pending_curve  = false;          // from our "# type bezier_curve"

        // Faces of the current object, bucketed by material (one Object per bucket).
        std::vector<std::string>             matOrder;
        std::unordered_map<std::string, int> matIndex;
        std::vector<std::vector<PendingFace>> matFaces;

        auto flushObject = [&]() {
            for (size_t b = 0; b < matFaces.size(); ++b) {
                auto& faces = matFaces[b];
                if (faces.empty()) continue;
                const std::string& matname = matOrder[b];

                core::Object obj;
                obj.type = core::ObjectType::MESH;
                core::Mesh& mesh = *obj.mesh;

                std::unordered_map<int,int> vmap, tmap, nmap, gmap;
                auto mapPos = [&](int g) -> int {
                    if (g < 0 || g >= (int)positions.size()) return 0;
                    auto it = vmap.find(g); if (it != vmap.end()) return it->second;
                    int l = (int)mesh.vertices.size(); vmap[g] = l; mesh.vertices.push_back(positions[g]); return l;
                };
                auto mapUv = [&](int g) -> int {
                    if (g < 0 || g >= (int)uvs.size()) return -1;
                    auto it = tmap.find(g); if (it != tmap.end()) return it->second;
                    int l = (int)mesh.texcoords.size(); tmap[g] = l; mesh.texcoords.push_back(uvs[g]); return l;
                };
                auto mapNorm = [&](int g) -> int {
                    if (g < 0 || g >= (int)normals.size()) return -1;
                    auto it = nmap.find(g); if (it != nmap.end()) return it->second;
                    int l = (int)mesh.normals.size(); nmap[g] = l; mesh.normals.push_back(normals[g]); return l;
                };

                core::Material mat = resolveMaterial(matname, lib);
                mesh.materials.push_back(mat);
                obj.material = mat;
                obj.material.filled = true;   // meshes render solid

                for (auto& pf : faces) {
                    core::Face face;
                    face.material  = 0;
                    face.smoothing = pf.smoothing;
                    if (!pf.group.empty()) {
                        // group-name dedup is keyed by a rolling hash of insertion;
                        // simplest: linear lookup over the small per-object set.
                        int gi = -1;
                        for (size_t k = 0; k < mesh.groups.size(); ++k)
                            if (mesh.groups[k] == pf.group) { gi = (int)k; break; }
                        if (gi < 0) { gi = (int)mesh.groups.size(); mesh.groups.push_back(pf.group); }
                        face.group = gi;
                    }
                    for (auto& c : pf.corners) {
                        core::FaceVertex lc;
                        lc.v  = mapPos(c.v);
                        lc.vt = mapUv(c.vt);
                        lc.vn = mapNorm(c.vn);
                        face.corners.push_back(lc);
                    }
                    // Fan-triangulate (faces are planar & convex from typical exporters).
                    for (size_t k = 1; k + 1 < face.corners.size(); ++k)
                        mesh.tri_indices.emplace_back((uint32_t)face.corners[0].v,
                                                      (uint32_t)face.corners[k].v,
                                                      (uint32_t)face.corners[k + 1].v);
                    mesh.faces.push_back(std::move(face));
                }

                std::string nm = current_object.empty()
                    ? (matname.empty() ? "mesh" : matname) : current_object;
                if (matFaces.size() > 1 && !matname.empty()) nm += ":" + matname;
                obj.name = nm;

                core::MeshFactory mf(std::move(obj));
                em.add(mf);
                res.object_count++;
            }
            matOrder.clear(); matIndex.clear(); matFaces.clear();
        };

        auto bucketFor = [&](const std::string& m) -> std::vector<PendingFace>& {
            auto it = matIndex.find(m);
            if (it != matIndex.end()) return matFaces[it->second];
            int idx = (int)matFaces.size();
            matIndex[m] = idx; matOrder.push_back(m); matFaces.emplace_back();
            return matFaces[idx];
        };

        // Resolves face/element tokens to world points for our own primitive path.
        auto resolvePoints = [&](std::istringstream& iss) {
            RawPts pts;
            std::string tok;
            while (iss >> tok) {
                try {
                    int idx = std::stoi(tok);  // index before any '/'
                    if (idx < 0) idx = (int)positions.size() + idx + 1;
                    if (idx > 0 && idx <= (int)positions.size()) {
                        const auto& p = positions[idx - 1];
                        pts.emplace_back(p.x, p.y, p.z);
                    }
                } catch (...) {}
            }
            return pts;
        };

        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line[0] == '#') {
                std::istringstream css(line.substr(1));
                std::string tag; css >> tag;
                if (tag == "color") {
                    int r, g, b, a = 255;
                    if (css >> r >> g >> b) { css >> a; }
                    pending_color = IM_COL32(r, g, b, a);
                } else if (tag == "filled") {
                    int v = 0; css >> v; pending_filled = (v != 0);
                } else if (tag == "type") {
                    std::string t; css >> t; pending_curve = (t == "bezier_curve");
                }
                continue;
            }
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string type; iss >> type;

            if (type == "v") {
                float x, y, z = 0.0f; iss >> x >> y >> z;
                positions.emplace_back(x, y, z);
            } else if (type == "vt") {
                core::TexCoord t; iss >> t.u >> t.v >> t.w;
                uvs.push_back(t);
            } else if (type == "vn") {
                float x, y, z = 0.0f; iss >> x >> y >> z;
                normals.emplace_back(x, y, z);
            } else if (type == "mtllib") {
                std::string lib_name; iss >> lib_name;
                if (!lib_name.empty()) {
                    mtl::Result mr = mtl::Load(base_dir + lib_name, lib);
                    if (!mr.ok) res.warnings.push_back("mtllib not found: " + lib_name);
                }
            } else if (type == "usemtl") {
                iss >> current_material;
            } else if (type == "g") {
                iss >> current_group;
            } else if (type == "s") {
                std::string s; iss >> s;
                current_smoothing = (s == "off" || s.empty()) ? 0 : std::atoi(s.c_str());
            } else if (type == "o") {
                flushObject();
                iss >> current_object;
                current_group.clear();
                pending_color = IM_COL32_WHITE;
                pending_curve = false;
            } else if (type == "p") {
                importPoint(current_object, resolvePoints(iss), pending_color, em);
                if (!positions.empty()) res.object_count++;
            } else if (type == "l") {
                auto pts = resolvePoints(iss);
                if (pending_curve) { importCurve2D(current_object, pts, pending_color, em); }
                else               { importWireframe(current_object, pts, pending_color, em); }
                res.object_count++;
            } else if (type == "f") {
                if (pending_filled) {
                    // Our own polygon (preceded by "# filled") — keep as a POLYGON.
                    importPolygon(current_object, resolvePoints(iss), pending_color, true, em);
                    pending_filled = false;
                    res.object_count++;
                } else {
                    // External mesh face — bucket by current material.
                    PendingFace pf; pf.group = current_group; pf.smoothing = current_smoothing;
                    std::string tok;
                    while (iss >> tok)
                        pf.corners.push_back(parseCorner(tok, (int)positions.size(),
                                                         (int)uvs.size(), (int)normals.size()));
                    if (pf.corners.size() >= 3) bucketFor(current_material).push_back(std::move(pf));
                }
            }
        }
        flushObject();  // remaining faces of the last object
        return res;
    }

    // ─── Export ──────────────────────────────────────────────────────────────────

    static void unpack_color(ImU32 col, int& r, int& g, int& b, int& a) {
        unsigned int uc = (unsigned int)col;
        r = (uc >>  0) & 0xFF;
        g = (uc >>  8) & 0xFF;
        b = (uc >> 16) & 0xFF;
        a = (uc >> 24) & 0xFF;
    }

    // Emits a full mesh object: v/vt/vn pools then faces with usemtl/g/s runs.
    static void exportMesh(std::ofstream& f, const core::Object& obj,
                           int& vi, int& vti, int& vni) {
        const core::Mesh& mesh = *obj.mesh;
        f << "o " << obj.name << "\n";
        const int v0 = vi, t0 = vti, n0 = vni;

        for (const auto& v : mesh.vertices) {
            core::Point w = obj.transform * v;
            f << "v " << w.x << " " << w.y << " " << w.z << "\n"; vi++;
        }
        for (const auto& t : mesh.texcoords) { f << "vt " << t.u << " " << t.v << "\n"; vti++; }
        for (const auto& nn : mesh.normals)  { f << "vn " << nn.x << " " << nn.y << " " << nn.z << "\n"; vni++; }

        int prevMat = -2, prevGroup = -2, prevSmooth = INT_MIN;
        for (const auto& face : mesh.faces) {
            if (face.material != prevMat) {
                if (face.material >= 0 && face.material < (int)mesh.materials.size()) {
                    const std::string& mn = mesh.materials[face.material].name;
                    if (!mn.empty()) f << "usemtl " << mn << "\n";
                }
                prevMat = face.material;
            }
            if (face.group != prevGroup) {
                if (face.group >= 0 && face.group < (int)mesh.groups.size())
                    f << "g " << mesh.groups[face.group] << "\n";
                prevGroup = face.group;
            }
            if (face.smoothing != prevSmooth) {
                f << "s " << (face.smoothing ? std::to_string(face.smoothing) : "off") << "\n";
                prevSmooth = face.smoothing;
            }
            f << "f";
            for (const auto& c : face.corners) {
                f << " " << (v0 + c.v);
                if (c.vt >= 0 || c.vn >= 0) {
                    f << "/";
                    if (c.vt >= 0) f << (t0 + c.vt);
                    if (c.vn >= 0) f << "/" << (n0 + c.vn);
                }
            }
            f << "\n";
        }
    }

    // Emits one of our in-app primitives using the legacy #-comment convention.
    static void exportPrimitive(std::ofstream& f, const core::Object& obj,
                                EntityManager& em, int& vi) {
        int r, g, b, a;
        unpack_color(obj.material.color, r, g, b, a);
        f << "o " << obj.name << "\n";
        f << "# color " << r << " " << g << " " << b << " " << a << "\n";

        std::vector<core::Point> wv;
        wv.reserve(obj.mesh->vertices.size());
        for (const auto& v : obj.mesh->vertices) wv.push_back(obj.transform * v);

        switch (obj.type) {
            case core::ObjectType::POINT: {
                if (wv.empty()) break;
                f << "v " << wv[0].x << " " << wv[0].y << " " << wv[0].z << "\n";
                f << "p " << vi << "\n"; vi++;
                break;
            }
            case core::ObjectType::LINE: {
                if (wv.size() < 2) break;
                f << "v " << wv[0].x << " " << wv[0].y << " " << wv[0].z << "\n";
                f << "v " << wv[1].x << " " << wv[1].y << " " << wv[1].z << "\n";
                f << "l " << vi << " " << vi+1 << "\n"; vi += 2;
                break;
            }
            case core::ObjectType::WIREFRAME: {
                for (const auto& v : wv)
                    f << "v " << v.x << " " << v.y << " " << v.z << "\n";
                f << "l";
                for (size_t i = 0; i < wv.size(); ++i) f << " " << vi + i;
                f << "\n"; vi += (int)wv.size();
                break;
            }
            case core::ObjectType::POLYGON: {
                f << "# filled " << (obj.material.filled ? 1 : 0) << "\n";
                size_t n = wv.size();
                if (n > 1 && wv.front().x == wv.back().x && wv.front().y == wv.back().y) n--;
                for (size_t i = 0; i < n; ++i)
                    f << "v " << wv[i].x << " " << wv[i].y << " " << wv[i].z << "\n";
                f << "f";
                for (size_t i = 0; i < n; ++i) f << " " << vi + i;
                f << "\n"; vi += (int)n;
                break;
            }
            case core::ObjectType::CURVE2D: {
                f << "# type bezier_curve\n";
                const CurveMetadata* meta = em.getCurveMetadata(obj.id);
                if (meta) {
                    for (const auto& p : meta->control_points) {
                        core::Point w = obj.transform * p;
                        f << "v " << w.x << " " << w.y << " " << w.z << "\n";
                    }
                    f << "l";
                    for (size_t i = 0; i < meta->control_points.size(); ++i) f << " " << vi + i;
                    f << "\n"; vi += (int)meta->control_points.size();
                }
                break;
            }
            default: break;
        }
    }

    void Export(std::ofstream& f, const std::vector<core::Object>& objects,
                EntityManager& em, int& vi) {
        int vti = 1, vni = 1;  // global vt/vn running indices (independent of v)
        for (const auto& obj : objects) {
            if (obj.type == core::ObjectType::MESH) exportMesh(f, obj, vi, vti, vni);
            else                                    exportPrimitive(f, obj, em, vi);
        }
    }
}
