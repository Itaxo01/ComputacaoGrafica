#include "Renderer.hpp"
#include "RendererBackground.hpp"
#include "RendererPreview.hpp"
#include "Window.hpp"
#include "AppConfig.hpp"
#include "RendererClipping.hpp"
#include "RendererTransform.hpp"
#include "RasterizationEngine.hpp"
#include "ParallelUtils.hpp"
#include "imgui.h"
#include <cmath>
#include <limits>
#include <cstdio>

void Renderer::draw_name_if_visible(const std::string& name, const core::Point& anchor) {
    core::Point ncs_anchor = window.GetWindowNCSMatrix() * anchor;
    if (ncs_anchor.x >= -1.0f && ncs_anchor.x <= 1.0f &&
        ncs_anchor.y >= -1.0f && ncs_anchor.y <= 1.0f) {
        core::Point p = window.NCSToViewport(ncs_anchor);
        auto cp = viewport.GetCanvasP();
        p.x += cp.first.x;
        p.y += cp.first.y;
        draw_list->AddText(ImVec2(p.x, p.y - 15), IM_COL32_WHITE, name.c_str());
    }
}

void Renderer::RenderBackground() {
    ::RenderBackground(draw_list, window, viewport);
}

void Renderer::DrawPreview() {
    const auto& pts = displayFile.getPreviewPoints();
    if (pts.empty()) return;

    core::ObjectType mode = displayFile.getPreviewMode();
    if (mode == core::ObjectType::POINT) return;

    auto ncs_mat = window.GetWindowNCSMatrix();
    ImVec2 offset = viewport.GetCanvasP().first;

    switch (mode) {
        case core::ObjectType::LINE:
        case core::ObjectType::WIREFRAME:
            DrawPreviewPolyline(draw_list, pts, ncs_mat, window, offset); break;
        case core::ObjectType::POLYGON:
            DrawPreviewPolygon(draw_list, pts, ncs_mat, window, offset);  break;
        case core::ObjectType::CURVE2D: {
            int method = displayFile.getPreviewMethod();
            switch (method) {
                case 0: DrawPreviewCurve2DBezier(draw_list, pts, ncs_mat, window, offset); break;
                case 1: DrawPreviewCurve2DBSpline(draw_list, pts, ncs_mat, window, offset); break;
                default: DrawPreviewCurve2DBezier(draw_list, pts, ncs_mat, window, offset); break;
            }
            break;
        }
        default: break;
    }
}

void Renderer::ProcessPreClipping() {
    const bool shading = Lighting::mode != Lighting::NONE;
    // Flatten every object into the SoA GeometryBuffer (positions still in model
    // space, plus the ObjectSlice table), then bake the matrices in per-vertex.
    BuildGeometryBuffer(displayFile.getObjects(), geom, slices, shading);
    if (AppConfig::is3d && AppConfig::perspective) {
        // Split pipeline: transform to VRC, clip against the near plane (before
        // the divide), then project + scale to NCS (divide happens here).
        TransformFlat(geom, slices, window.GetVRCMatrix(), shading);
        geom = NearClipFlat(geom, window.GetNearPlaneZ());
        ProjectFlat(geom, window.GetProjectionScaleMatrix());
    } else {
        TransformFlat(geom, slices, window.GetWindowNCSMatrix(), shading);
    }
}

void Renderer::ApplyClipping() {
    auto [clip_min, clip_max] = window.getClipBoundsNCS();
    geom = BoxClipFlat(geom, slices, clip_min, clip_max, AppConfig::clipping_mode);
}

void Renderer::ApplyViewportTransform() {
    ViewportFlat(geom, window, (float)AppConfig::supersample);
}

void Renderer::GenerateDrawList() {
    WindowAttributes w = window.getWindowAttributes();
    auto canvas_p = viewport.GetCanvasP();
    if (refresh_cache || rendererCache.cache_changed(w, canvas_p.first, canvas_p.second)) {
        log.AddLog("Scene changed, refreshing object cache\n");
        rendererCache.store_cache(w, canvas_p.first, canvas_p.second);
        refresh_cache = false;
        ProcessPreClipping();
        ApplyClipping();
        // Gather + depth-sort filled triangles in NCS space (z still meaningful)
        // before the viewport map drops z; lines/points use the viewport verts.
        BuildSortedTrianglesFlat(geom, slices, window, (float)AppConfig::supersample, sortedTris);
        ApplyViewportTransform();
    }
}

void Renderer::RasterizeFramebuffer() {
    const int H = framebuffer.Height();
    if (H <= 0 || framebuffer.Width() <= 0) return;

    const bool zt = AppConfig::z_buffer;
    const bool less = !AppConfig::depth_ascending; // nearer-is-smaller unless flipped
    const int  ss = AppConfig::supersample;        // line/point sizes scale with SSAA
    const float farZ = less ?  std::numeric_limits<float>::infinity()
                            : -std::numeric_limits<float>::infinity();

    // One band per hardware thread (TBB pool when available). Each band clears
    // and fills a disjoint row range: solid triangles (depth-tested when z_buffer
    // is on, else in painter's order), then wireframe lines / points on top.
    cg_parallel_chunks((std::size_t)H, [&](std::size_t lo, std::size_t hi) {
        int y_lo = (int)lo, y_hi = (int)hi;
        framebuffer.ClearRows(y_lo, y_hi, 0u); // transparent: grid shows through
        if (zt) framebuffer.ClearDepthRows(y_lo, y_hi, farZ);

        for (const auto& t : sortedTris)
            DrawTriangleFilled(framebuffer, t.a, t.b, t.c, t.za, t.zb, t.zc,
                               t.P, t.N, t.mat, t.color, shadeCtx, zt, less, y_lo, y_hi);

        // Wireframe lines and points, straight from the flat buffer (color via the
        // owning ObjectSlice). Filled triangles were drawn above from sortedTris.
        const size_t nL = geom.lineCount();
        for (size_t l = 0; l < nL; ++l) {
            core::Point a = geom.getPos(geom.lineIdx[2*l]);
            core::Point b = geom.getPos(geom.lineIdx[2*l+1]);
            ImU32 col = slices[geom.lineObj[l]].color;
            DrawLine(framebuffer, a.x, a.y, a.z, b.x, b.y, b.z, col, ss, zt, less, y_lo, y_hi);
        }
        const size_t nP = geom.pointCount();
        for (size_t p = 0; p < nP; ++p) {
            core::Point v = geom.getPos(geom.pointIdx[p]);
            ImU32 col = slices[geom.pointObj[p]].color;
            DrawPoint(framebuffer, v.x, v.y, v.z, col, ss, zt, less, y_lo, y_hi);
        }
    });
}

void Renderer::render() {
    draw_list = viewport.GetDrawList();
    RenderBackground();
    GenerateDrawList();

    // Build per-frame shading inputs. These are read live (not cached), so moving a
    // light or orbiting updates the lighting immediately without a geometry rebuild.
    effectiveLights.clear();
    for (const auto& L : Lighting::lights) effectiveLights.push_back(L);
    if (Lighting::headlight) {
        core::Light hl;
        hl.position  = window.GetEyeWorld();
        hl.color     = Lighting::headlight_color;
        hl.intensity = Lighting::headlight_intensity;
        effectiveLights.push_back(hl);
    }
    shadeCtx.mode    = Lighting::mode;
    shadeCtx.eye     = window.GetEyeWorld();
    shadeCtx.ambient = Lighting::ambient;
    shadeCtx.lights  = &effectiveLights;

    // Rasterize the scene into the CPU framebuffer, then blit it over the grid.
    ImVec2 sz = viewport.GetCanvasSize();
    auto canvas_p = viewport.GetCanvasP();
    framebuffer.Resize((int)std::lround(sz.x), (int)std::lround(sz.y), AppConfig::supersample);
    RasterizeFramebuffer();
    framebuffer.Resolve(); // CPU box-downsample (premultiplied) into display resolution
    framebuffer.Present(draw_list, canvas_p.first, canvas_p.second);

    if (AppConfig::render_names) {
        for (const auto& obj : displayFile.getObjects())
            draw_name_if_visible(obj.name, obj.anchorPoint());
    }

    DrawPreview();

    // Viewport resolution readout (bottom-left), drawn on top of the scene like the
    // other overlays so the framebuffer blit doesn't cover it.
    char res[64];
    std::snprintf(res, sizeof(res), "%d x %d px  (SSAA x%d)",
                  (int)(sz.x + 0.5f), (int)(sz.y + 0.5f), AppConfig::supersample);
    ImVec2 res_sz = ImGui::CalcTextSize(res);
    draw_list->AddText(ImVec2(canvas_p.first.x + 6.0f, canvas_p.second.y - res_sz.y - 6.0f),
                       IM_COL32(210, 210, 210, 220), res);
}

void Renderer::notifyTransformation() {
    refresh_cache = true;
}
