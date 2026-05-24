#include "RendererBackground.hpp"
#include "RendererClipping.hpp"
#include "AppConfig.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

static inline ImVec2 ToImVec2(const core::Point &p) { return ImVec2(p.x, p.y); }

// Grid step calculation similar to GeoGebra
static float calculate_step(float width) {
    float raw_step = width / 10.0f;
    float exp      = std::floor(std::log10(raw_step));
    float mag      = std::pow(10.0f, exp);
    float fraction = raw_step / mag;
    float mult = 1.0f;
    if      (fraction >= 1.5f && fraction < 3.5f) mult = 2.0f;
    else if (fraction >= 3.5f && fraction < 7.5f) mult = 5.0f;
    else if (fraction >= 7.5f)                    mult = 10.0f;
    return mult * mag;
}

void RenderBackground(ImDrawList* draw_list, const Window& window, const Viewport& viewport) {
    auto canvas_p = viewport.GetCanvasP();
    ImVec2 canvas_p0 = canvas_p.first;
    ImVec2 canvas_p1 = canvas_p.second;

    const float border = 2.0f;
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(ImVec2(canvas_p0.x - border, canvas_p0.y - border),
                       ImVec2(canvas_p1.x + border, canvas_p1.y + border),
                       IM_COL32(255, 255, 255, 255), 0.0f, border * 2);

    WindowAttributes w_attr = window.getWindowAttributes();
    core::Point ncs_min(-1.0f, -1.0f, 0.0f);
    core::Point ncs_max( 1.0f,  1.0f, 0.0f);
    auto ncs_mat = window.GetWindowNCSMatrix();

    // Project a world-space segment through NCS, clip it, return screen endpoints.
    auto project_line = [&](core::Point wa, core::Point wb,
                            core::Point &sa, core::Point &sb) -> bool {
        core::Line l(wa, wb);
        l.a = ncs_mat * l.a;
        l.b = ncs_mat * l.b;
        if (!ClipLine(l, ncs_min, ncs_max)) return false;
        sa = window.NCSToViewport(l.a); sa.x += canvas_p0.x; sa.y += canvas_p0.y;
        sb = window.NCSToViewport(l.b); sb.x += canvas_p0.x; sb.y += canvas_p0.y;
        return true;
    };

    // Snap a label to the nearest visible point on a grid line toward the relevant axis.
    auto label_pos = [&](const core::Point& axis_pt,
                         const core::Point& seg_a_ncs,
                         const core::Point& seg_b_ncs) -> core::Point {
        core::Point target_ncs = ncs_mat * axis_pt;
        core::Point dir = seg_b_ncs - seg_a_ncs;
        float len2 = dir.x * dir.x + dir.y * dir.y;
        core::Point result_ncs = target_ncs;
        if (len2 > 1e-6f) {
            core::Point pa = target_ncs - seg_a_ncs;
            float t = std::clamp((pa.x * dir.x + pa.y * dir.y) / len2, 0.001f, 0.999f);
            result_ncs = seg_a_ncs + (dir * t);
        }
        core::Point sp = window.NCSToViewport(result_ncs);
        sp.x += canvas_p0.x;
        sp.y += canvas_p0.y;
        return sp;
    };

    float max_r = std::sqrt(w_attr.width * w_attr.width + w_attr.height * w_attr.height) / 2.0f;

    if (AppConfig::is3d) {
        // B proportional to view_width — box appears at fixed screen size regardless of zoom.
        // Box centered at VRP so panning doesn't move the box on screen.
        const float B  = w_attr.width * 0.31f;
        const float cx = w_attr.center.x;  // VRP.x
        const float cy = w_attr.center.y;  // VRP.y
        const float cz = w_attr.center.z;  // VRP.z

        // ── Bounding box (12 edges) centered at VRP ───────────────────────────────
        // Y-up: "floor" = low Y, "ceiling" = high Y, side edges run along Y.
        auto box_edge = [&](core::Point a, core::Point b) {
            core::Point sa, sb;
            if (project_line(a, b, sa, sb))
                draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(200, 200, 220, 255), 1.5f);
        };
        // floor face (y = cy-B) — XZ square at low Y
        box_edge({cx-B,cy-B,cz-B},{cx+B,cy-B,cz-B}); box_edge({cx+B,cy-B,cz-B},{cx+B,cy-B,cz+B});
        box_edge({cx+B,cy-B,cz+B},{cx-B,cy-B,cz+B}); box_edge({cx-B,cy-B,cz+B},{cx-B,cy-B,cz-B});
        // ceiling face (y = cy+B)
        box_edge({cx-B,cy+B,cz-B},{cx+B,cy+B,cz-B}); box_edge({cx+B,cy+B,cz-B},{cx+B,cy+B,cz+B});
        box_edge({cx+B,cy+B,cz+B},{cx-B,cy+B,cz+B}); box_edge({cx-B,cy+B,cz+B},{cx-B,cy+B,cz-B});
        // vertical edges along Y
        box_edge({cx-B,cy-B,cz-B},{cx-B,cy+B,cz-B}); box_edge({cx+B,cy-B,cz-B},{cx+B,cy+B,cz-B});
        box_edge({cx+B,cy-B,cz+B},{cx+B,cy+B,cz+B}); box_edge({cx-B,cy-B,cz+B},{cx-B,cy+B,cz+B});

        // ── XZ floor grid (y = 0), spanning VRP±B in X and Z ─────────────────────
        if (viewport.show_grid) {
            float step = calculate_step(w_attr.width);
            char label[32];

            float gx0 = cx - B, gx1 = cx + B;
            float gz0 = cz - B, gz1 = cz + B;
            float start_x = std::ceil(gx0 / step) * step;
            float start_z = std::ceil(gz0 / step) * step;

            // Lines parallel to X (constant z, y=0) — label at the left endpoint (gx0 side)
            for (float z = start_z; z <= gz1 + step * 0.01f; z += step) {
                core::Point sa, sb;
                if (!project_line({gx0, 0, z}, {gx1, 0, z}, sa, sb)) continue;
                draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(80, 80, 80, 255));
                float dz = z - cz;
                if (viewport.show_axis_coordinates && std::abs(dz) > step * 0.1f) {
                    snprintf(label, sizeof(label), "z=%.2f", dz);
                    draw_list->AddText(
                        ImVec2(std::clamp(sa.x + 4, canvas_p0.x + 2.0f, canvas_p1.x - 40.0f),
                               std::clamp(sa.y - 14, canvas_p0.y + 2.0f, canvas_p1.y - 15.0f)),
                        IM_COL32(200, 200, 200, 255), label);
                }
            }

            // Lines parallel to Z (constant x, y=0) — label at the near endpoint (gz0 side)
            for (float x = start_x; x <= gx1 + step * 0.01f; x += step) {
                core::Point sa, sb;
                if (!project_line({x, 0, gz0}, {x, 0, gz1}, sa, sb)) continue;
                draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(80, 80, 80, 255));
                float dx = x - cx;
                if (viewport.show_axis_coordinates && std::abs(dx) > step * 0.1f) {
                    snprintf(label, sizeof(label), "x=%.2f", dx);
                    draw_list->AddText(
                        ImVec2(std::clamp(sa.x + 4, canvas_p0.x + 2.0f, canvas_p1.x - 40.0f),
                               std::clamp(sa.y + 4, canvas_p0.y + 2.0f, canvas_p1.y - 15.0f)),
                        IM_COL32(200, 200, 200, 255), label);
                }
            }

            // Bounding square of the XZ floor grid
            auto floor_edge_sq = [&](core::Point a, core::Point b) {
                core::Point sa, sb;
                if (project_line(a, b, sa, sb))
                    draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(160, 160, 185, 240), 1.5f);
            };
            floor_edge_sq({gx0,0,gz0},{gx1,0,gz0}); floor_edge_sq({gx1,0,gz0},{gx1,0,gz1});
            floor_edge_sq({gx1,0,gz1},{gx0,0,gz1}); floor_edge_sq({gx0,0,gz1},{gx0,0,gz0});

            if (viewport.show_axis_coordinates) {
                core::Point on = ncs_mat * core::Point(0, 0, 0);
                if (on.x >= -1.0f && on.x <= 1.0f && on.y >= -1.0f && on.y <= 1.0f) {
                    core::Point os = window.NCSToViewport(on);
                    draw_list->AddText(ImVec2(os.x + canvas_p0.x - 12, os.y + canvas_p0.y + 4),
                                       IM_COL32(200, 200, 200, 255), "0");
                }
            }
        }

        // ── Axes centered at VRP: X (red), Y (green = up), Z (blue) ─────────────
        if (viewport.show_axes) {
            core::Point sa, sb;
            auto axis_label = [&](core::Point tip_world, const char* lbl, ImU32 col) {
                core::Point tip = ncs_mat * tip_world;
                if (tip.x >= -1.0f && tip.x <= 1.0f && tip.y >= -1.0f && tip.y <= 1.0f) {
                    core::Point ts = window.NCSToViewport(tip);
                    draw_list->AddText(ImVec2(ts.x + canvas_p0.x + 4, ts.y + canvas_p0.y - 8), col, lbl);
                }
            };
            if (project_line({cx-B,cy,cz},{cx+B,cy,cz}, sa, sb)) {
                draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(220, 60, 60, 255), 2.0f);
                axis_label({cx+B, cy, cz}, "X", IM_COL32(220, 60, 60, 255));
            }
            if (project_line({cx,cy-B,cz},{cx,cy+B,cz}, sa, sb)) {
                draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(60, 220, 60, 255), 2.0f);
                axis_label({cx, cy+B, cz}, "Y", IM_COL32(60, 220, 60, 255));
            }
            if (project_line({cx,cy,cz-B},{cx,cy,cz+B}, sa, sb)) {
                draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(60, 100, 220, 255), 2.0f);
                axis_label({cx, cy, cz+B}, "Z", IM_COL32(60, 100, 220, 255));
            }
        }

    } else {
        // ── 2D: XY-plane grid ───────────────────────────────────────────────────────
        if (viewport.show_grid) {
            float step = calculate_step(w_attr.width);
            char label[32];

            float cx = w_attr.center.x, cy = w_attr.center.y;
            float x0 = std::floor((cx - max_r) / step) * step, x1 = cx + max_r;
            float y0 = std::floor((cy - max_r) / step) * step, y1 = cy + max_r;

            // Vertical grid lines (constant x)
            for (float x = x0; x <= x1; x += step) {
                core::Point pa(x, y0), pb(x, y1);
                core::Line l(pa, pb);
                l.a = ncs_mat * l.a; l.b = ncs_mat * l.b;
                if (!ClipLine(l, ncs_min, ncs_max)) continue;

                core::Point ts = window.NCSToViewport(l.b), bs = window.NCSToViewport(l.a);
                ts.x += canvas_p0.x; ts.y += canvas_p0.y;
                bs.x += canvas_p0.x; bs.y += canvas_p0.y;
                draw_list->AddLine(ToImVec2(ts), ToImVec2(bs), IM_COL32(100, 100, 100, 255));

                if (viewport.show_axis_coordinates && std::abs(x) > step * 0.1f) {
                    snprintf(label, sizeof(label), "%.2f", x);
                    core::Point sp = label_pos(core::Point(x, 0, 0), l.a, l.b);
                    draw_list->AddText(
                        ImVec2(std::clamp(sp.x + 6, canvas_p0.x + 2.0f, canvas_p1.x - 30.0f),
                               std::clamp(sp.y + 4, canvas_p0.y + 2.0f, canvas_p1.y - 15.0f)),
                        IM_COL32(200, 200, 200, 255), label);
                }
            }

            // Horizontal grid lines (constant y)
            for (float y = y0; y <= y1; y += step) {
                core::Point pa(x0, y), pb(x1, y);
                core::Line l(pa, pb);
                l.a = ncs_mat * l.a; l.b = ncs_mat * l.b;
                if (!ClipLine(l, ncs_min, ncs_max)) continue;

                core::Point rs = window.NCSToViewport(l.b), ls = window.NCSToViewport(l.a);
                rs.x += canvas_p0.x; rs.y += canvas_p0.y;
                ls.x += canvas_p0.x; ls.y += canvas_p0.y;
                draw_list->AddLine(ToImVec2(rs), ToImVec2(ls), IM_COL32(100, 100, 100, 255));

                if (viewport.show_axis_coordinates && std::abs(y) > step * 0.1f) {
                    snprintf(label, sizeof(label), "%.2f", y);
                    core::Point sp = label_pos(core::Point(0, y, 0), l.a, l.b);
                    draw_list->AddText(
                        ImVec2(std::clamp(sp.x + 6,  canvas_p0.x + 2.0f,  canvas_p1.x - 30.0f),
                               std::clamp(sp.y - 16, canvas_p0.y + 15.0f, canvas_p1.y - 15.0f)),
                        IM_COL32(200, 200, 200, 255), label);
                }
            }

            if (viewport.show_axis_coordinates) {
                core::Point on = ncs_mat * core::Point(0, 0, 0);
                if (on.x >= -1.0f && on.x <= 1.0f && on.y >= -1.0f && on.y <= 1.0f) {
                    core::Point os = window.NCSToViewport(on);
                    draw_list->AddText(ImVec2(os.x + canvas_p0.x - 12, os.y + canvas_p0.y + 4),
                                       IM_COL32(200, 200, 200, 255), "0");
                }
            }
        }

        // ── 2D axes ─────────────────────────────────────────────────────────────────
        if (viewport.show_axes) {
            core::Point cx = w_attr.center;

            core::Point pay(0, cx.y + max_r, 0), pby(0, cx.y - max_r, 0);
            core::Line l_y(pay, pby);
            l_y.a = ncs_mat * l_y.a; l_y.b = ncs_mat * l_y.b;
            if (ClipLine(l_y, ncs_min, ncs_max)) {
                core::Point ts = window.NCSToViewport(l_y.a), bs = window.NCSToViewport(l_y.b);
                ts.x += canvas_p0.x; ts.y += canvas_p0.y;
                bs.x += canvas_p0.x; bs.y += canvas_p0.y;
                draw_list->AddLine(ToImVec2(ts), ToImVec2(bs), IM_COL32(100, 100, 100, 255), 2.0f);
            }

            core::Point pax(cx.x + max_r, 0, 0), pbx(cx.x - max_r, 0, 0);
            core::Line l_x(pax, pbx);
            l_x.a = ncs_mat * l_x.a; l_x.b = ncs_mat * l_x.b;
            if (ClipLine(l_x, ncs_min, ncs_max)) {
                core::Point rs = window.NCSToViewport(l_x.a), ls = window.NCSToViewport(l_x.b);
                rs.x += canvas_p0.x; rs.y += canvas_p0.y;
                ls.x += canvas_p0.x; ls.y += canvas_p0.y;
                draw_list->AddLine(ToImVec2(rs), ToImVec2(ls), IM_COL32(100, 100, 100, 255), 2.0f);
            }
        }
    }
}
