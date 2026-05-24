#include "Renderer.hpp"
#include "RendererBackground.hpp"
#include "RendererPreview.hpp"
#include "Window.hpp"
#include "AppConfig.hpp"
#include "RendererClipping.hpp"
#include "RendererTransform.hpp"
#include "imgui.h"

static inline ImVec2 ToImVec2(const core::Point& p) { return ImVec2(p.x, p.y); }

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

void Renderer::DrawObject(const core::Object& obj) {
    const ImU32 col   = obj.material.color;
    const float width = 2.0f;

    // Point: single vertex, no indices
    if (obj.type == core::ObjectType::POINT) {
        if (!obj.mesh->vertices.empty()) {
            const auto& v = obj.mesh->vertices[0];
            const float h = 1.0f;
            draw_list->AddRectFilled(ImVec2(v.x-h, v.y-h), ImVec2(v.x+h, v.y+h),
                                     col, 2.0f, ImDrawFlags_RoundCornersAll);
        }
        return;
    }

    // Draw line segments
    for (const auto& [i, j] : obj.mesh->line_indices) {
        draw_list->AddLine(ToImVec2(obj.mesh->vertices[i]),
                           ToImVec2(obj.mesh->vertices[j]),
                           col, width);
    }

    // Draw filled triangles
    for (const auto& [ti, tj, tk] : obj.mesh->tri_indices) {
        draw_list->AddTriangleFilled(
            ToImVec2(obj.mesh->vertices[ti]),
            ToImVec2(obj.mesh->vertices[tj]),
            ToImVec2(obj.mesh->vertices[tk]),
            col);
    }
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

void Renderer::ApplyNCSTransform() {
    drawObjects = displayFile.getObjects(); // deep copy for transform/clip
    auto ncs_mat = window.GetWindowNCSMatrix();
    TransformToNCS(drawObjects, ncs_mat);
}

void Renderer::ApplyClipping() {
    core::Point ncs_min(-1.0f, -1.0f, 0.0f);
    core::Point ncs_max( 1.0f,  1.0f, 0.0f);
    ClipObjects(drawObjects, ncs_min, ncs_max, viewport.GetClippingMode());
}

void Renderer::ApplyViewportTransform() {
    auto canvas_p = viewport.GetCanvasP();
    ImVec2 offset = canvas_p.first;
    TransformToViewport(drawObjects, window, offset);
}

void Renderer::GenerateDrawList() {
    WindowAttributes w = window.getWindowAttributes();
    auto canvas_p = viewport.GetCanvasP();
    if (refresh_cache || rendererCache.cache_changed(w, canvas_p.first, canvas_p.second)) {
        log.AddLog("Scene changed, refreshing object cache\n");
        rendererCache.store_cache(w, canvas_p.first, canvas_p.second);
        refresh_cache = false;
        ApplyNCSTransform();
        ApplyClipping();
        ApplyViewportTransform();
    }
}

void Renderer::render() {
    draw_list = viewport.GetDrawList();
    RenderBackground();
    GenerateDrawList();

    for (const auto& obj : drawObjects)
        DrawObject(obj);

    if (AppConfig::render_names) {
        for (const auto& obj : displayFile.getObjects())
            draw_name_if_visible(obj.name, obj.anchorPoint());
    }

    DrawPreview();
}

void Renderer::notifyTransformation() {
    refresh_cache = true;
}
