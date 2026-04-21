#include "RendererPreview.hpp"
#include "Point.hpp"

static ImVec2 to_screen(float wx, float wy, const core::mat4 &ncs_mat, const Window &window, ImVec2 offset) {
    core::Point ncs = ncs_mat * core::Point(wx, wy, 0.0f);
    core::Point vp  = window.NCSToViewport(ncs);
    return ImVec2(vp.x + offset.x, vp.y + offset.y);
}

// ─── Polyline (Line / Wireframe) ─────────────────────────────────────────────

void DrawPreviewPolyline(ImDrawList *dl, const PreviewPts &pts, const core::mat4 &ncs_mat, const Window &window, ImVec2 offset) {
    constexpr ImU32 COL_EDGE   = IM_COL32(200, 200, 200, 200);
    constexpr ImU32 COL_RUBBER = IM_COL32(200, 200, 200, 120);
    constexpr ImU32 COL_VERTEX = IM_COL32(255, 220,  80, 220);

    for (size_t i = 1; i < pts.size(); i++) {
        auto [x0, y0, z0] = pts[i - 1];
        auto [x1, y1, z1] = pts[i];
        dl->AddLine(to_screen(x0, y0, ncs_mat, window, offset),
                    to_screen(x1, y1, ncs_mat, window, offset), COL_EDGE, 1.5f);
    }

    ImVec2 mouse = ImGui::GetMousePos();
    auto [lx, ly, lz] = pts.back();
    dl->AddLine(to_screen(lx, ly, ncs_mat, window, offset), mouse, COL_RUBBER, 1.0f);

    for (const auto &[px, py, pz] : pts)
        dl->AddCircleFilled(to_screen(px, py, ncs_mat, window, offset), 3.5f, COL_VERTEX);
}

// ─── Polygon ─────────────────────────────────────────────────────────────────

void DrawPreviewPolygon(ImDrawList *dl, const PreviewPts &pts, const core::mat4 &ncs_mat, const Window &window, ImVec2 offset) {
    constexpr ImU32 COL_EDGE   = IM_COL32(200, 200, 200, 200);
    constexpr ImU32 COL_RUBBER = IM_COL32(200, 200, 200, 120);
    constexpr ImU32 COL_CLOSE  = IM_COL32(100, 200, 255, 100);
    constexpr ImU32 COL_VERTEX = IM_COL32(255, 220,  80, 220);

    for (size_t i = 1; i < pts.size(); i++) {
        auto [x0, y0, z0] = pts[i - 1];
        auto [x1, y1, z1] = pts[i];
        dl->AddLine(to_screen(x0, y0, ncs_mat, window, offset),
                    to_screen(x1, y1, ncs_mat, window, offset), COL_EDGE, 1.5f);
    }

    ImVec2 mouse = ImGui::GetMousePos();
    auto [lx, ly, lz] = pts.back();
    dl->AddLine(to_screen(lx, ly, ncs_mat, window, offset), mouse, COL_RUBBER, 1.0f);

    if (pts.size() >= 2) {
        auto [fx, fy, fz] = pts.front();
        dl->AddLine(mouse, to_screen(fx, fy, ncs_mat, window, offset), COL_CLOSE, 1.0f);
    }

    for (const auto &[px, py, pz] : pts)
        dl->AddCircleFilled(to_screen(px, py, ncs_mat, window, offset), 3.5f, COL_VERTEX);
}

// ─── Bezier Curve ─────────────────────────────────────────────────────────────

void DrawPreviewBezier(ImDrawList *dl, const PreviewPts &pts, const core::mat4 &ncs_mat, const Window &window, ImVec2 offset) {
    constexpr ImU32 COL_CURVE  = IM_COL32(100, 220, 255, 220);
    constexpr ImU32 COL_ANCHOR = IM_COL32(255, 220,  80, 220);
    constexpr ImU32 COL_CTRL   = IM_COL32(255, 150,  50, 200);
    constexpr ImU32 COL_HANDLE = IM_COL32(200, 200, 200, 130);
    constexpr ImU32 COL_RUBBER = IM_COL32(200, 200, 200, 100);

    int n = (int)pts.size();

    auto lerp2 = [](ImVec2 a, ImVec2 b, float t) -> ImVec2 {
        return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    };

    // Evaluated curve for each complete cubic segment
    int num_segs = (n >= 4) ? (n - 1) / 3 : 0;
    for (int seg = 0; seg < num_segs; seg++) {
        ImVec2 p[4];
        for (int j = 0; j < 4; j++) {
            auto [x, y, z] = pts[seg * 3 + j];
            p[j] = to_screen(x, y, ncs_mat, window, offset);
        }
        const int steps = 40;
        ImVec2 prev = p[0];
        for (int i = 1; i <= steps; i++) {
            float t = (float)i / (float)steps;
            ImVec2 q[4] = {p[0], p[1], p[2], p[3]};
            for (int d = 3; d > 0; d--)
                for (int j = 0; j < d; j++)
                    q[j] = lerp2(q[j], q[j + 1], t);
            dl->AddLine(prev, q[0], COL_CURVE, 2.0f);
            prev = q[0];
        }
    }

    // Handle lines: ctrl point → its anchor
    for (int i = 0; i < n; i++) {
        if (i % 3 == 1) {
            auto [cx, cy, cz] = pts[i];
            auto [ax, ay, az] = pts[i - 1];
            dl->AddLine(to_screen(ax, ay, ncs_mat, window, offset),
                        to_screen(cx, cy, ncs_mat, window, offset), COL_HANDLE, 1.0f);
        } else if (i % 3 == 2 && i + 1 < n) {
            auto [cx, cy, cz] = pts[i];
            auto [ax, ay, az] = pts[i + 1];
            dl->AddLine(to_screen(ax, ay, ncs_mat, window, offset),
                        to_screen(cx, cy, ncs_mat, window, offset), COL_HANDLE, 1.0f);
        }
    }

    // Vertex dots
    for (int i = 0; i < n; i++) {
        auto [px, py, pz] = pts[i];
        if (i % 3 == 0)
            dl->AddCircleFilled(to_screen(px, py, ncs_mat, window, offset), 4.0f, COL_ANCHOR);
        else
            dl->AddCircleFilled(to_screen(px, py, ncs_mat, window, offset), 2.5f, COL_CTRL);
    }

    // Rubber-band from last placed point to mouse
    ImVec2 mouse = ImGui::GetMousePos();
    auto [lx, ly, lz] = pts.back();
    dl->AddLine(to_screen(lx, ly, ncs_mat, window, offset), mouse, COL_RUBBER, 1.0f);
}
