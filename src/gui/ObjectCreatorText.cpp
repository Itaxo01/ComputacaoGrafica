#include "ObjectCreatorText.hpp"
#include <cctype>
#include <cstdlib>
#include <cstring>

// ── Parsing ───────────────────────────────────────────────────────────────────

void ObjectCreatorText::reparse() {
    parsed.clear();
    parse_ok = false;
    parse_error.clear();

    const char* p = buffer;

    auto skipws = [&]() {
        while (*p && std::isspace((unsigned char)*p)) ++p;
    };
    // ';' separates surface matrix rows/patches; treat it like any other point
    // separator so the flat point list can still be collected uniformly.
    auto skip_sep = [&]() {
        while (*p && (std::isspace((unsigned char)*p) || *p == ',' || *p == ';')) ++p;
    };

    skip_sep();
    if (*p == '\0') return;

    int idx = 0;
    while (*p != '\0') {
        if (*p != '(') {
            parse_error = "Expected '(' before point " + std::to_string(idx + 1);
            parsed.clear(); return;
        }
        ++p;

        skipws();
        char* end;
        float x = std::strtof(p, &end);
        if (end == p) { parse_error = "Expected x in point " + std::to_string(idx + 1); parsed.clear(); return; }
        p = end; skipws();
        if (*p != ',') { parse_error = "Expected ',' after x in point " + std::to_string(idx + 1); parsed.clear(); return; }
        ++p;

        skipws();
        float y = std::strtof(p, &end);
        if (end == p) { parse_error = "Expected y in point " + std::to_string(idx + 1); parsed.clear(); return; }
        p = end; skipws();

        // z is optional
        float z = 0.0f;
        if (*p == ',') {
            ++p; skipws();
            z = std::strtof(p, &end);
            if (end == p) { parse_error = "Expected z in point " + std::to_string(idx + 1); parsed.clear(); return; }
            p = end; skipws();
        }

        if (*p != ')') { parse_error = "Expected ')' after point " + std::to_string(idx + 1); parsed.clear(); return; }
        ++p;

        parsed.emplace_back(x, y, z);
        ++idx;
        skip_sep();
    }

    if (!parsed.empty()) parse_ok = true;
}

// ── Validation ────────────────────────────────────────────────────────────────

bool ObjectCreatorText::validate() const {
    if (!parse_ok || parsed.empty()) return false;
    int n = (int)parsed.size();
    switch (mode) {
        case core::ObjectType::POINT:     return n == 1;
        case core::ObjectType::LINE:      return n == 2;
        case core::ObjectType::WIREFRAME: return n >= 2;
        case core::ObjectType::POLYGON:   return n >= 3;
        case core::ObjectType::CURVE2D:
            if (method == 0) return n >= 4 && (n - 1) % 3 == 0;
            if (method == 1) return n >= 4;
            return false;
        case core::ObjectType::SURFACE:
            return n > 0 && n % 16 == 0; // k patches of 16 control points each
        default: return false;
    }
}

// ── Strings ───────────────────────────────────────────────────────────────────

const char* ObjectCreatorText::format_hint() const {
    switch (mode) {
        case core::ObjectType::POINT:
            return "(x,y)  or  (x,y,z)";
        case core::ObjectType::LINE:
            return "(x1,y1),(x2,y2)";
        case core::ObjectType::WIREFRAME:
        case core::ObjectType::POLYGON:
            return "(x1,y1),(x2,y2),(x3,y3), ...";
        case core::ObjectType::CURVE2D:
            if (method == 0)
                return "(P0),(C0),(C1),(P1),(C2),(C3),(P2), ...  — anchor,ctrl,ctrl,anchor,...";
            return "(P0),(P1),(P2),(P3), ...  — 4+ control points";
        case core::ObjectType::SURFACE:
            return "(x11,y11,z11),(x12,y12,z12),...;(x21,...),...  — 16 points/patch, rows split by ';'";
        default:
            return "(x,y)  or  (x,y,z)";
    }
}

std::string ObjectCreatorText::validation_msg() const {
    int n = (int)parsed.size();
    switch (mode) {
        case core::ObjectType::POINT:     return "needs exactly 1 point (got "     + std::to_string(n) + ")";
        case core::ObjectType::LINE:      return "needs exactly 2 points (got "    + std::to_string(n) + ")";
        case core::ObjectType::WIREFRAME: return "needs at least 2 points (got "   + std::to_string(n) + ")";
        case core::ObjectType::POLYGON:   return "needs at least 3 points (got "   + std::to_string(n) + ")";
        case core::ObjectType::CURVE2D:
            if (method == 0) return "Bezier: needs 4,7,10... points — anchor,ctrl,ctrl,anchor (got " + std::to_string(n) + ")";
            return "B-Spline: needs at least 4 control points (got " + std::to_string(n) + ")";
        case core::ObjectType::SURFACE:
            return "needs 16 control points per patch — k*16 total (got " + std::to_string(n) + ")";
        default: return "";
    }
}

// ── Mode selector ─────────────────────────────────────────────────────────────

void ObjectCreatorText::draw_mode_selector() {
    // 2D/3D selector — independent of the app's mode, so a 2D object can be typed
    // in while the app is in 3D (and vice-versa). Flipping it may invalidate the
    // current mode, so snap back to Point when that happens.
    int dim = target_3d ? 1 : 0;
    ImGui::TextUnformatted("Object set:"); ImGui::SameLine();
    if (ImGui::RadioButton("2D##octdim", &dim, 0)) target_3d = false;
    ImGui::SameLine();
    if (ImGui::RadioButton("3D##octdim", &dim, 1)) target_3d = true;
    if (!core::typeAvailableInMode(mode, target_3d)) mode = core::ObjectType::POINT;
    ImGui::Separator();

    auto type_radio = [&](const char* label, core::ObjectType t, int sub_method = -1) {
        bool selected = (mode == t) && (sub_method < 0 || method == sub_method);
        if (ImGui::RadioButton(label, selected)) {
            mode = t;
            if (sub_method >= 0) method = sub_method;
        }
    };

    type_radio("Point##oct", core::ObjectType::POINT);     ImGui::SameLine();
    type_radio("Line##oct", core::ObjectType::LINE);       ImGui::SameLine();
    type_radio("Wireframe##oct", core::ObjectType::WIREFRAME); ImGui::SameLine();
    float polygon_x = ImGui::GetCursorPosX();
    type_radio("Polygon##oct", core::ObjectType::POLYGON);

    if (!target_3d) {
        ImGui::SameLine();
        type_radio("Curve##oct", core::ObjectType::CURVE2D);
    } else {
        ImGui::SameLine();
        type_radio("Bezier Surf##oct", core::ObjectType::SURFACE, 0); ImGui::SameLine();
        type_radio("B-Spline Surf##oct", core::ObjectType::SURFACE, 1);
    }

    // Polygon: filled toggle
    if (mode == core::ObjectType::POLYGON) {
        ImGui::SetCursorPosX(polygon_x);
        ImGui::Checkbox("Filled##oct", &filled);
    }

    // Curve: method sub-selector
    if (mode == core::ObjectType::CURVE2D) {
        ImGui::SetCursorPosX(polygon_x);
        ImGui::RadioButton("Bezier##oct",   &method, 0); ImGui::SameLine();
        ImGui::RadioButton("B-Spline##oct", &method, 1);
    }
}

// ── Modal ─────────────────────────────────────────────────────────────────────

void ObjectCreatorText::Open(core::ObjectType initial_mode, int initial_method,
                             bool initial_filled, bool initial_3d) {
    mode      = initial_mode;
    method    = initial_method;
    filled    = initial_filled;
    target_3d = initial_3d;
    // Make sure the seeded mode is actually offered by the seeded 2D/3D set.
    if (!core::typeAvailableInMode(mode, target_3d)) mode = core::ObjectType::POINT;
    open_requested = true;
}

void ObjectCreatorText::OpenForEdit(core::ObjectType initial_mode, int initial_method,
                                     bool initial_filled,
                                     const std::vector<std::tuple<float, float, float>> &pts) {
    char tmp[BUF_SIZE]; tmp[0] = '\0';
    size_t off = 0;
    // One point per line: ImGui's multiline input doesn't wrap long lines, so we
    // break each point onto its own line to keep large point lists readable.
    for (const auto& [x, y, z] : pts) {
        int w;
        if (z != 0.0f) w = snprintf(tmp + off, sizeof(tmp) - off, "(%g,%g,%g),\n", x, y, z);
        else           w = snprintf(tmp + off, sizeof(tmp) - off, "(%g,%g),\n",     x, y);
        if (w > 0 && off + (size_t)w < sizeof(tmp)) off += (size_t)w;
    }
    // Strip the trailing separator (",\n").
    while (off > 0 && (tmp[off - 1] == '\n' || tmp[off - 1] == ',' || tmp[off - 1] == ' '))
        tmp[--off] = '\0';
    memcpy(buffer, tmp, sizeof(buffer));
    // Seed the 2D/3D selector so the object being edited is visible in it.
    bool want_3d = !core::typeAvailableInMode(initial_mode, false);
    Open(initial_mode, initial_method, initial_filled, want_3d);
}

bool ObjectCreatorText::DrawModal(std::vector<std::tuple<float, float, float>> &out_points,
                                   core::ObjectType &out_mode, int &out_method, bool &out_filled) {
    if (open_requested) {
        ImGui::OpenPopup(popup_title);
        open_requested = false;
    }

    bool confirmed = false;

    ImGui::SetNextWindowSize(ImVec2(560, 420), ImGuiCond_Always);
    if (!ImGui::BeginPopupModal(popup_title, nullptr,
                                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
        return false;

    // ── Mode selector ────────────────────────────────────────────────────────
    draw_mode_selector();
    ImGui::Separator();

    // ── Format hint + Clear ──────────────────────────────────────────────────
    ImGui::TextDisabled("Format: %s", format_hint());
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Clear").x - 4);
    if (ImGui::SmallButton("Clear")) {
        buffer[0] = '\0';
        parsed.clear();
        parse_ok = false;
        parse_error.clear();
    }
    ImGui::Separator();

    // ── Text input ───────────────────────────────────────────────────────────
    ImGui::InputTextMultiline("##txt", buffer, BUF_SIZE, ImVec2(-1, 180));
    reparse();

    ImGui::Spacing();

    // ── Live feedback ────────────────────────────────────────────────────────
    if (buffer[0] == '\0') {
        ImGui::TextDisabled("Enter coordinates above...");
    } else if (!parse_ok) {
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Error: %s", parse_error.c_str());
    } else if (validate()) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                           "%d point(s) — ready to create", (int)parsed.size());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.0f, 1.0f),
                           "%d point(s) — %s", (int)parsed.size(), validation_msg().c_str());
    }

    // ── Confirm / Cancel ─────────────────────────────────────────────────────
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 36);
    ImGui::Separator();

    bool can_confirm = validate();
    if (!can_confirm) ImGui::BeginDisabled();
    if (ImGui::Button("Confirm", ImVec2(120, 0))) {
        out_points = parsed;
        out_mode   = mode;
        out_method = method;
        out_filled = filled;
        confirmed  = true;
        buffer[0]  = '\0';
        parsed.clear();
        parse_ok = false;
        ImGui::CloseCurrentPopup();
    }
    if (!can_confirm) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0)))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
    return confirmed;
}
