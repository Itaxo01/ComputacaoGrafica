#include "ObjectController.hpp"
#include <algorithm>
#include <utility>
#include <cmath>
#include <cctype>
#include <sstream>
#include <fstream>
#include <string>

// Avalia a transformação numa fração "alpha" do caminho interpolando os
// parâmetros: translação e ângulo escalam linearmente; escala interpola de 1→s.
core::mat4 Transformation::matrixAt(float alpha) const {
    switch (kind) {
        case TransformKind::TRANSLATE:
            return core::getTranslationMatrix(vec.x * alpha, vec.y * alpha, vec.z * alpha);
        case TransformKind::SCALE:
            return core::getScalingMatrix(1.0f + (vec.x - 1.0f) * alpha,
                                          1.0f + (vec.y - 1.0f) * alpha,
                                          1.0f + (vec.z - 1.0f) * alpha);
        case TransformKind::ROTATE:
            return core::getRotationMatrixAroundAxisCenteredAt(angle * alpha, axis, vec);
        default:
            return core::mat4(true);
    }
}

void ObjectController::HandleAddScaling(float x, float y, float z) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Scaling: (%.1f, %.1f, %.1f)", x, y, z);
    for (long long id : selected_ids) {
        Transformation t{id_counter, core::getScalingMatrix(x, y, z), buffer};
        t.kind = TransformKind::SCALE;
        t.vec = {x, y, z};
        transformation_buffer[id].push_back(t);
    }
    id_counter++;
}

void ObjectController::HandleAddTranslation(float x, float y, float z) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Translation: (%.1f, %.1f, %.1f)", x, y, z);
    for (long long id : selected_ids) {
        Transformation t{id_counter, core::getTranslationMatrix(x, y, z), buffer};
        t.kind = TransformKind::TRANSLATE;
        t.vec = {x, y, z};
        transformation_buffer[id].push_back(t);
    }
    id_counter++;
}

void ObjectController::HandleAddRotation(float cx, float cy, float cz,
                                         float ax, float ay, float az, float angle) {
    char* buffer = new char[128];
    snprintf(buffer, 128, "Rotation: %.1f deg @axis(%.1f, %.1f, %.1f)", angle, ax, ay, az);
    core::Point center{cx, cy, cz};
    core::Point axis{ax, ay, az};
    for (long long id : selected_ids) {
        Transformation t{id_counter,
                         core::getRotationMatrixAroundAxisCenteredAt(angle, axis, center),
                         buffer};
        t.kind = TransformKind::ROTATE;
        t.vec = center;
        t.axis = axis;
        t.angle = angle;
        transformation_buffer[id].push_back(t);
    }
    id_counter++;
}

void ObjectController::ApplyTransformations(float duration, bool loop) {
    for (long long id : selected_ids) {
        if (!entityManager.exists(id)) continue;   // ignora seleções obsoletas
        auto& buffer = transformation_buffer[id];
        if (buffer.empty()) continue;

        if (duration <= 0.0f) {
            // Aplicação instantânea (comportamento antigo).
            core::mat4 final_matrix(true);
            for (const auto& t : buffer) {
                final_matrix = t.matrix * final_matrix;
            }
            entityManager.ApplyTransformation(id, final_matrix);
        } else {
            // Inicia (ou substitui) uma animação para este objeto.
            ObjectAnimation anim;
            anim.base = entityManager.getObject(id).transform;
            anim.steps = buffer;
            anim.duration = duration;
            anim.elapsed = 0.0f;
            anim.loop = loop;
            active_animations[id] = std::move(anim);
        }
        buffer.clear();
    }
}

void ObjectController::Update(float dt) {
    if (active_animations.empty()) return;

    std::vector<long long> finished;
    for (auto& [id, anim] : active_animations) {
        // O objeto pode ter sido removido enquanto a animação corria (ex.: o donut
        // em loop). Sem isto, getObject(id) lançaria std::out_of_range.
        if (!entityManager.exists(id)) { finished.push_back(id); continue; }

        anim.elapsed += dt;
        float alpha = anim.duration > 0.0f ? anim.elapsed / anim.duration : 1.0f;

        // Numa animação em loop, "embrulha" o tempo decorrido em vez de parar.
        // Como uma rotação completa (360°) volta à pose base, o loop é contínuo.
        if (anim.loop && anim.duration > 0.0f) {
            anim.elapsed = std::fmod(anim.elapsed, anim.duration);
            alpha = anim.elapsed / anim.duration;
        } else if (alpha >= 1.0f) {
            alpha = 1.0f;
        }

        // Recompõe a combinação dos passos avaliados em "alpha", na mesma ordem
        // de ApplyTransformations. Em alpha=1 isto iguala exatamente a matriz final.
        core::mat4 combined(true);
        for (const auto& t : anim.steps) {
            combined = t.matrixAt(alpha) * combined;
        }
        entityManager.SetTransformation(id, combined * anim.base);

        if (!anim.loop && alpha >= 1.0f) finished.push_back(id);
    }

    for (long long id : finished) active_animations.erase(id);
}

void ObjectController::TransformationIntersection() {
    tranformation_intersection.clear();
    if (selected_ids.empty()) {
        return;
    }

    std::vector<Transformation> base = transformation_buffer[*selected_ids.begin()];
    std::vector<Transformation> intersection;

    for (long long id : selected_ids) {
        if (id == *selected_ids.begin()) {
            continue;
        }

        intersection.clear();
        for (const auto& t1 : base) {
            for (const auto& t2 : transformation_buffer[id]) {
                if (t1.id == t2.id) { // ADICIONAR ID PARA TRANSFOMAÇÃO
                    intersection.push_back(t1);
                    //break;
                }
            }
        }
        base = intersection;
    }

    tranformation_intersection = base;
}

// Por enquanto está quadrático. Depois fazer algoritmo melhor usando hash.
// Reaproveitar metodo Transformation Intersection depois também.
const std::vector<char*> ObjectController::GetTransformationBufferNames() {
    if (selected_ids.empty()) {
        return {};
    }

    std::vector<Transformation> base = transformation_buffer[*selected_ids.begin()];
    std::vector<Transformation> intersection;

    for (long long id : selected_ids) {
        if (id == *selected_ids.begin()) {
            continue;
        }

        intersection.clear();
        for (const auto& t1 : base) {
            for (const auto& t2 : transformation_buffer[id]) {
                if (t1.id == t2.id) { // ADICIONAR ID PARA TRANSFOMAÇÃO
                    intersection.push_back(t1);
                    break;
                }
            }
        }
        base = intersection;
    }

    std::vector<char*> names;
    for (const auto& t : base) {
        names.push_back(t.description);
    }

    return names;
}

// ── Importação de transformações por script de texto ───────────────────────────

static std::string str_lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

std::string ObjectController::ApplyTransformationScript(const std::string& text) {
    if (selected_ids.empty()) return "No object selected.";

    float duration = 0.0f;
    bool  loop = false;

    // O script define o conjunto completo de transformações: limpa qualquer buffer
    // parcial dos objetos selecionados para um resultado determinístico.
    for (long long id : selected_ids) transformation_buffer[id].clear();

    std::istringstream in(text);
    std::string rawline;
    int lineno = 0;
    while (std::getline(in, rawline)) {
        lineno++;
        auto hash = rawline.find('#');                 // comentários
        if (hash != std::string::npos) rawline.erase(hash);

        std::istringstream ls(rawline);
        std::string cmd;
        if (!(ls >> cmd)) continue;                    // linha vazia
        cmd = str_lower(cmd);

        auto err = [&](const std::string& m) {
            return "Line " + std::to_string(lineno) + ": " + m;
        };

        if (cmd == "duration") {
            if (!(ls >> duration)) return err("expected a number after 'duration'");
        } else if (cmd == "loop") {
            std::string v; loop = true;
            if (ls >> v) { v = str_lower(v); loop = (v != "off" && v != "0" && v != "false"); }
        } else if (cmd == "translate") {
            float x, y, z = 0.0f;
            if (!(ls >> x >> y)) return err("translate needs at least 'x y'");
            ls >> z;
            HandleAddTranslation(x, y, z);
        } else if (cmd == "scale") {
            float sx, sy, sz = 1.0f;
            if (!(ls >> sx >> sy)) return err("scale needs at least 'sx sy'");
            ls >> sz;
            HandleAddScaling(sx, sy, sz);
        } else if (cmd == "rotate") {
            float angle;
            if (!(ls >> angle)) return err("rotate needs an angle");
            float ax = 0, ay = 0, az = 1;              // eixo default: Z
            float cx = 0, cy = 0, cz = 0;
            int aroundMode = 0;                        // 0 itself, 1 origin, 2 point
            std::string kw;
            while (ls >> kw) {
                kw = str_lower(kw);
                if (kw == "axis") {
                    if (!(ls >> ax >> ay >> az)) return err("'axis' needs 3 numbers");
                } else if (kw == "around") {
                    std::string where;
                    if (!(ls >> where)) return err("'around' needs a target");
                    where = str_lower(where);
                    if (where == "itself")      aroundMode = 0;
                    else if (where == "origin") aroundMode = 1;
                    else if (where == "point") {
                        aroundMode = 2;
                        if (!(ls >> cx >> cy >> cz)) return err("'around point' needs 'x y z'");
                    } else return err("unknown 'around' target '" + where + "'");
                } else return err("unexpected token '" + kw + "'");
            }
            if (aroundMode == 0) {
                auto [x, y, z] = GetSelectedObjectsCenter();
                cx = x; cy = y; cz = z;
            } else if (aroundMode == 1) {
                cx = cy = cz = 0.0f;
            }
            HandleAddRotation(cx, cy, cz, ax, ay, az, angle);
        } else {
            return err("unknown command '" + cmd + "'");
        }
    }

    ApplyTransformations(duration, loop);
    return "";
}

std::string ObjectController::ApplyTransformationScriptFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "Could not open file: " + path;
    std::stringstream ss;
    ss << f.rdbuf();
    return ApplyTransformationScript(ss.str());
}