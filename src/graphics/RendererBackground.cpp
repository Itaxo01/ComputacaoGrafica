#include "RendererBackground.hpp"
#include "RendererClipping.hpp"
#include "AppConfig.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

static inline ImVec2 ToImVec2(const core::Point &p) { return ImVec2(p.x, p.y); }

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

// ── Shared context passed to all rendering helpers ────────────────────────────

struct BgCtx {
    ImDrawList*      draw_list;
    const Window&    window;
    const Viewport&  viewport;
    WindowAttributes w_attr;
    core::mat4       ncs_mat;
    core::Point      ncs_min;
    core::Point      ncs_max;
    ImVec2           canvas_p0;
    ImVec2           canvas_p1;
    float            max_r;
    float            B;          // bounding box half-size (3D)
    float            cx, cy, cz; // bounding box center = VRP (3D)
};

static bool ProjectLine(const BgCtx& ctx,
                        core::Point wa, core::Point wb,
                        core::Point& sa, core::Point& sb) {
    core::Line l(wa, wb);
    l.a = ctx.ncs_mat * l.a;
    l.b = ctx.ncs_mat * l.b;
    if (!ClipLine(l, ctx.ncs_min, ctx.ncs_max)) return false;
    sa = ctx.window.NCSToViewport(l.a); sa.x += ctx.canvas_p0.x; sa.y += ctx.canvas_p0.y;
    sb = ctx.window.NCSToViewport(l.b); sb.x += ctx.canvas_p0.x; sb.y += ctx.canvas_p0.y;
    return true;
}

static core::Point LabelPos(const BgCtx& ctx,
                             const core::Point& axis_pt,
                             const core::Point& seg_a_ncs,
                             const core::Point& seg_b_ncs) {
    core::Point target_ncs = ctx.ncs_mat * axis_pt;
    core::Point dir = seg_b_ncs - seg_a_ncs;
    float len2 = dir.x * dir.x + dir.y * dir.y;
    core::Point result_ncs = target_ncs;
    if (len2 > 1e-6f) {
        core::Point pa = target_ncs - seg_a_ncs;
        float t = std::clamp((pa.x * dir.x + pa.y * dir.y) / len2, 0.001f, 0.999f);
        result_ncs = seg_a_ncs + (dir * t);
    }
    core::Point sp = ctx.window.NCSToViewport(result_ncs);
    sp.x += ctx.canvas_p0.x;
    sp.y += ctx.canvas_p0.y;
    return sp;
}

// ── Rendering components ──────────────────────────────────────────────────────

static void RenderBoundingBox(const BgCtx& ctx) {
    if (!AppConfig::is3d || !ctx.viewport.show_bounding_box) return;

    const float B = ctx.B, cx = ctx.cx, cy = ctx.cy, cz = ctx.cz;

    auto edge = [&](core::Point a, core::Point b) {
        core::Point sa, sb;
        if (ProjectLine(ctx, a, b, sa, sb))
            ctx.draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(200, 200, 220, 255), 1.5f);
    };
    auto floor_edge = [&](core::Point a, core::Point b) {
        core::Point sa, sb;
        if (ProjectLine(ctx, a, b, sa, sb))
            ctx.draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(160, 160, 185, 240), 1.5f);
    };

    // 12 box edges
    edge({cx-B,cy-B,cz-B},{cx+B,cy-B,cz-B}); edge({cx+B,cy-B,cz-B},{cx+B,cy-B,cz+B});
    edge({cx+B,cy-B,cz+B},{cx-B,cy-B,cz+B}); edge({cx-B,cy-B,cz+B},{cx-B,cy-B,cz-B});
    edge({cx-B,cy+B,cz-B},{cx+B,cy+B,cz-B}); edge({cx+B,cy+B,cz-B},{cx+B,cy+B,cz+B});
    edge({cx+B,cy+B,cz+B},{cx-B,cy+B,cz+B}); edge({cx-B,cy+B,cz+B},{cx-B,cy+B,cz-B});
    edge({cx-B,cy-B,cz-B},{cx-B,cy+B,cz-B}); edge({cx+B,cy-B,cz-B},{cx+B,cy+B,cz-B});
    edge({cx+B,cy-B,cz+B},{cx+B,cy+B,cz+B}); edge({cx-B,cy-B,cz+B},{cx-B,cy+B,cz+B});

    // XZ floor outline
    float gx0 = cx - B, gx1 = cx + B, gz0 = cz - B, gz1 = cz + B;
    floor_edge({gx0,0,gz0},{gx1,0,gz0}); floor_edge({gx1,0,gz0},{gx1,0,gz1});
    floor_edge({gx1,0,gz1},{gx0,0,gz1}); floor_edge({gx0,0,gz1},{gx0,0,gz0});
}

static void RenderGrid(const BgCtx& ctx) {
    if (!ctx.viewport.show_grid) return;

    if (AppConfig::is3d) {
        const float B = ctx.B, cx = ctx.cx, cz = ctx.cz;
        float step = calculate_step(ctx.w_attr.width);
        char label[32];

        float gx0 = cx - B, gx1 = cx + B;
        float gz0 = cz - B, gz1 = cz + B;

        for (float z = std::ceil(gz0 / step) * step; z <= gz1 + step * 0.01f; z += step) {
            core::Point sa, sb;
            if (!ProjectLine(ctx, {gx0, 0, z}, {gx1, 0, z}, sa, sb)) continue;
            ctx.draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(80, 80, 80, 255));
            if (ctx.viewport.show_axis_coordinates && std::abs(z - cz) > step * 0.1f) {
                snprintf(label, sizeof(label), "z=%.2f", z - cz);
                ctx.draw_list->AddText(
                    ImVec2(std::clamp(sa.x + 4, ctx.canvas_p0.x + 2.0f, ctx.canvas_p1.x - 40.0f),
                           std::clamp(sa.y - 14, ctx.canvas_p0.y + 2.0f, ctx.canvas_p1.y - 15.0f)),
                    IM_COL32(200, 200, 200, 255), label);
            }
        }

        for (float x = std::ceil(gx0 / step) * step; x <= gx1 + step * 0.01f; x += step) {
            core::Point sa, sb;
            if (!ProjectLine(ctx, {x, 0, gz0}, {x, 0, gz1}, sa, sb)) continue;
            ctx.draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(80, 80, 80, 255));
            if (ctx.viewport.show_axis_coordinates && std::abs(x - cx) > step * 0.1f) {
                snprintf(label, sizeof(label), "x=%.2f", x - cx);
                ctx.draw_list->AddText(
                    ImVec2(std::clamp(sa.x + 4, ctx.canvas_p0.x + 2.0f, ctx.canvas_p1.x - 40.0f),
                           std::clamp(sa.y + 4, ctx.canvas_p0.y + 2.0f, ctx.canvas_p1.y - 15.0f)),
                    IM_COL32(200, 200, 200, 255), label);
            }
        }

        if (ctx.viewport.show_axis_coordinates) {
            core::Point on = ctx.ncs_mat * core::Point(0, 0, 0);
            if (on.x >= -1.0f && on.x <= 1.0f && on.y >= -1.0f && on.y <= 1.0f) {
                core::Point os = ctx.window.NCSToViewport(on);
                ctx.draw_list->AddText(
                    ImVec2(os.x + ctx.canvas_p0.x - 12, os.y + ctx.canvas_p0.y + 4),
                    IM_COL32(200, 200, 200, 255), "0");
            }
        }
    } else {
        float step = calculate_step(ctx.w_attr.width);
        char label[32];

        float wx = ctx.w_attr.center.x, wy = ctx.w_attr.center.y;
        float x0 = std::floor((wx - ctx.max_r) / step) * step, x1 = wx + ctx.max_r;
        float y0 = std::floor((wy - ctx.max_r) / step) * step, y1 = wy + ctx.max_r;

        for (float x = x0; x <= x1; x += step) {
            core::Point pa(x, y0), pb(x, y1);
            core::Line l(pa, pb);
            l.a = ctx.ncs_mat * l.a; l.b = ctx.ncs_mat * l.b;
            if (!ClipLine(l, ctx.ncs_min, ctx.ncs_max)) continue;
            core::Point ts = ctx.window.NCSToViewport(l.b), bs = ctx.window.NCSToViewport(l.a);
            ts.x += ctx.canvas_p0.x; ts.y += ctx.canvas_p0.y;
            bs.x += ctx.canvas_p0.x; bs.y += ctx.canvas_p0.y;
            ctx.draw_list->AddLine(ToImVec2(ts), ToImVec2(bs), IM_COL32(100, 100, 100, 255));
            if (ctx.viewport.show_axis_coordinates && std::abs(x) > step * 0.1f) {
                snprintf(label, sizeof(label), "%.2f", x);
                core::Point sp = LabelPos(ctx, core::Point(x, 0, 0), l.a, l.b);
                ctx.draw_list->AddText(
                    ImVec2(std::clamp(sp.x + 6, ctx.canvas_p0.x + 2.0f, ctx.canvas_p1.x - 30.0f),
                           std::clamp(sp.y + 4, ctx.canvas_p0.y + 2.0f, ctx.canvas_p1.y - 15.0f)),
                    IM_COL32(200, 200, 200, 255), label);
            }
        }

        for (float y = y0; y <= y1; y += step) {
            core::Point pa(x0, y), pb(x1, y);
            core::Line l(pa, pb);
            l.a = ctx.ncs_mat * l.a; l.b = ctx.ncs_mat * l.b;
            if (!ClipLine(l, ctx.ncs_min, ctx.ncs_max)) continue;
            core::Point rs = ctx.window.NCSToViewport(l.b), ls = ctx.window.NCSToViewport(l.a);
            rs.x += ctx.canvas_p0.x; rs.y += ctx.canvas_p0.y;
            ls.x += ctx.canvas_p0.x; ls.y += ctx.canvas_p0.y;
            ctx.draw_list->AddLine(ToImVec2(rs), ToImVec2(ls), IM_COL32(100, 100, 100, 255));
            if (ctx.viewport.show_axis_coordinates && std::abs(y) > step * 0.1f) {
                snprintf(label, sizeof(label), "%.2f", y);
                core::Point sp = LabelPos(ctx, core::Point(0, y, 0), l.a, l.b);
                ctx.draw_list->AddText(
                    ImVec2(std::clamp(sp.x + 6,  ctx.canvas_p0.x + 2.0f,  ctx.canvas_p1.x - 30.0f),
                           std::clamp(sp.y - 16, ctx.canvas_p0.y + 15.0f, ctx.canvas_p1.y - 15.0f)),
                    IM_COL32(200, 200, 200, 255), label);
            }
        }

        if (ctx.viewport.show_axis_coordinates) {
            core::Point on = ctx.ncs_mat * core::Point(0, 0, 0);
            if (on.x >= -1.0f && on.x <= 1.0f && on.y >= -1.0f && on.y <= 1.0f) {
                core::Point os = ctx.window.NCSToViewport(on);
                ctx.draw_list->AddText(
                    ImVec2(os.x + ctx.canvas_p0.x - 12, os.y + ctx.canvas_p0.y + 4),
                    IM_COL32(200, 200, 200, 255), "0");
            }
        }
    }
}

static void RenderAxes(const BgCtx& ctx) {
    if (!ctx.viewport.show_axes) return;

    if (AppConfig::is3d) {
        const float B = ctx.B, cx = ctx.cx, cy = ctx.cy, cz = ctx.cz;
        core::Point sa, sb;

        auto axis_label = [&](core::Point tip_world, const char* lbl, ImU32 col) {
            core::Point tip = ctx.ncs_mat * tip_world;
            if (tip.x >= -1.0f && tip.x <= 1.0f && tip.y >= -1.0f && tip.y <= 1.0f) {
                core::Point ts = ctx.window.NCSToViewport(tip);
                ctx.draw_list->AddText(
                    ImVec2(ts.x + ctx.canvas_p0.x + 4, ts.y + ctx.canvas_p0.y - 8), col, lbl);
            }
        };

        if (ProjectLine(ctx, {cx-B,cy,cz},{cx+B,cy,cz}, sa, sb)) {
            ctx.draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(220, 60, 60, 255), 2.0f);
            axis_label({cx+B, cy, cz}, "X", IM_COL32(220, 60, 60, 255));
        }
        if (ProjectLine(ctx, {cx,cy-B,cz},{cx,cy+B,cz}, sa, sb)) {
            ctx.draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(60, 220, 60, 255), 2.0f);
            axis_label({cx, cy+B, cz}, "Y", IM_COL32(60, 220, 60, 255));
        }
        if (ProjectLine(ctx, {cx,cy,cz-B},{cx,cy,cz+B}, sa, sb)) {
            ctx.draw_list->AddLine(ToImVec2(sa), ToImVec2(sb), IM_COL32(60, 100, 220, 255), 2.0f);
            axis_label({cx, cy, cz+B}, "Z", IM_COL32(60, 100, 220, 255));
        }
    } else {
        core::Point c = ctx.w_attr.center;

        core::Point pay(0, c.y + ctx.max_r, 0), pby(0, c.y - ctx.max_r, 0);
        core::Line l_y(pay, pby);
        l_y.a = ctx.ncs_mat * l_y.a; l_y.b = ctx.ncs_mat * l_y.b;
        if (ClipLine(l_y, ctx.ncs_min, ctx.ncs_max)) {
            core::Point ts = ctx.window.NCSToViewport(l_y.a), bs = ctx.window.NCSToViewport(l_y.b);
            ts.x += ctx.canvas_p0.x; ts.y += ctx.canvas_p0.y;
            bs.x += ctx.canvas_p0.x; bs.y += ctx.canvas_p0.y;
            ctx.draw_list->AddLine(ToImVec2(ts), ToImVec2(bs), IM_COL32(100, 100, 100, 255), 2.0f);
        }

        core::Point pax(c.x + ctx.max_r, 0, 0), pbx(c.x - ctx.max_r, 0, 0);
        core::Line l_x(pax, pbx);
        l_x.a = ctx.ncs_mat * l_x.a; l_x.b = ctx.ncs_mat * l_x.b;
        if (ClipLine(l_x, ctx.ncs_min, ctx.ncs_max)) {
            core::Point rs = ctx.window.NCSToViewport(l_x.a), ls = ctx.window.NCSToViewport(l_x.b);
            rs.x += ctx.canvas_p0.x; rs.y += ctx.canvas_p0.y;
            ls.x += ctx.canvas_p0.x; ls.y += ctx.canvas_p0.y;
            ctx.draw_list->AddLine(ToImVec2(rs), ToImVec2(ls), IM_COL32(100, 100, 100, 255), 2.0f);
        }
    }
}

// ── Public entry point ────────────────────────────────────────────────────────

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
    float max_r = std::sqrt(w_attr.width * w_attr.width + w_attr.height * w_attr.height) / 2.0f;

    BgCtx ctx {
        .draw_list = draw_list,
        .window    = window,
        .viewport  = viewport,
        .w_attr    = w_attr,
        .ncs_mat   = window.GetWindowNCSMatrix(),
        .ncs_min   = core::Point(-1.0f, -1.0f, 0.0f),
        .ncs_max   = core::Point( 1.0f,  1.0f, 0.0f),
        .canvas_p0 = canvas_p0,
        .canvas_p1 = canvas_p1,
        .max_r     = max_r,
        .B         = window.getBoundingBoxHalfSize(),
        .cx        = w_attr.center.x,
        .cy        = w_attr.center.y,
        .cz        = w_attr.center.z,
    };

    RenderBoundingBox(ctx);
    RenderGrid(ctx);
    RenderAxes(ctx);
}
