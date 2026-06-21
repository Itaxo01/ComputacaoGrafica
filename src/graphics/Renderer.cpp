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

void Renderer::DrawObject(const RenderedObject& obj, int y_lo, int y_hi) {
    const ImU32 col = obj.color;

    const int ss = AppConfig::supersample; // line/point sizes scale with the SSAA factor

    if (obj.type == core::ObjectType::POINT) {
        if (!obj.mesh.vertices.empty()) {
            const auto& v = obj.mesh.vertices[0];
            DrawPoint(framebuffer, v.x, v.y, col, ss, y_lo, y_hi);
        }
        return;
    }

    for (const auto& [i, j] : obj.mesh.line_indices) {
        const auto& a = obj.mesh.vertices[i];
        const auto& b = obj.mesh.vertices[j];
        DrawLine(framebuffer, a.x, a.y, b.x, b.y, col, ss, y_lo, y_hi);
    }
    // Filled triangles are drawn separately, globally depth-sorted (see RasterizeFramebuffer).
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
    if (AppConfig::is3d && AppConfig::perspective) {
        // Split pipeline: transform to VRC, clip against the near plane (before
        // the divide), then project + scale to NCS (divide happens here).
        TransformObjectAndDoNCS(drawObjects, displayFile.getObjects(), window.GetVRCMatrix());
        ClipNearPlane(drawObjects, window.GetNearPlaneZ());
        ProjectVertices(drawObjects, window.GetProjectionScaleMatrix());
    } else {
        TransformObjectAndDoNCS(drawObjects, displayFile.getObjects(), window.GetWindowNCSMatrix());
    }
}

void Renderer::ApplyClipping() {
    auto [clip_min, clip_max] = window.getClipBoundsNCS();
    ClipObjects(drawObjects, clip_min, clip_max, viewport.GetClippingMode());
}

void Renderer::ApplyViewportTransform() {
    TransformToViewport(drawObjects, window, (float)AppConfig::supersample);
}

void Renderer::GenerateDrawList() {
    WindowAttributes w = window.getWindowAttributes();
    auto canvas_p = viewport.GetCanvasP();
    // The cached vertices are pre-scaled by the SSAA factor, so a change to it
    // must rebuild them — the window/canvas cache key wouldn't catch it.
    if (AppConfig::supersample != last_supersample) {
        last_supersample = AppConfig::supersample;
        refresh_cache = true;
    }
    if (refresh_cache || rendererCache.cache_changed(w, canvas_p.first, canvas_p.second)) {
        log.AddLog("Scene changed, refreshing object cache\n");
        rendererCache.store_cache(w, canvas_p.first, canvas_p.second);
        refresh_cache = false;
        ProcessPreClipping();
        ApplyClipping();
        // Gather + depth-sort filled triangles in NCS space (z still meaningful)
        // before the viewport map drops z; lines/points use the viewport verts.
        BuildSortedTriangles(drawObjects, window, (float)AppConfig::supersample, sortedTris);
        ApplyViewportTransform();
    }
}

void Renderer::RasterizeFramebuffer() {
    const int H = framebuffer.Height();
    if (H <= 0 || framebuffer.Width() <= 0) return;

    // One band per hardware thread (TBB pool when available). Each band clears
    // and fills a disjoint row range: solid triangles first, back-to-front, then
    // wireframe lines / points on top — matching the old draw_list layering.
    cg_parallel_chunks((std::size_t)H, [&](std::size_t lo, std::size_t hi) {
        int y_lo = (int)lo, y_hi = (int)hi;
        framebuffer.ClearRows(y_lo, y_hi, 0u); // transparent: grid shows through

        for (const auto& t : sortedTris)
            DrawTriangleFilled(framebuffer, t.a, t.b, t.c, t.color, y_lo, y_hi);

        for (const auto& obj : drawObjects)
            DrawObject(obj, y_lo, y_hi);
    });
}

void Renderer::render() {
    draw_list = viewport.GetDrawList();
    RenderBackground();
    GenerateDrawList();

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
}

void Renderer::notifyTransformation() {
    refresh_cache = true;
}
