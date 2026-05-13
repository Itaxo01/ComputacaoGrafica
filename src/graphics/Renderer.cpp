#include "Renderer.hpp"
#include "RendererBackground.hpp"
#include "RendererPreview.hpp"
#include "Shape.hpp"
#include "Window.hpp"
#include "imgui.h"
#include <string>
#include "RendererClipping.hpp"
#include "RendererTransform.hpp"

inline ImVec2 ToImVec2(const core::Point &p) {
    return ImVec2(p.x, p.y);
}

#ifndef DONT_DRAW_SHAPE_NAME
    void Renderer::draw_name_if_visible(const core::Shape &shape){
        core::Point anchor(shape.anchorPoint());
            
        // Map anchor to NCS to see if it is visible on screen
        core::Point ncs_anchor = window.GetWindowNCSMatrix() * anchor;
        
        if (ncs_anchor.x >= -1.0f && ncs_anchor.x <= 1.0f && 
            ncs_anchor.y >= -1.0f && ncs_anchor.y <= 1.0f) {
            
            core::Point p = window.NCSToViewport(ncs_anchor);
            auto cp = viewport.GetCanvasP();
            p.x += cp.first.x;
            p.y += cp.first.y;

            const int magic_number = 15;
            ImVec2 pos(p.x, p.y - magic_number);
            draw_list->AddText(pos, IM_COL32_WHITE, shape.name.c_str());
        }
    }
#endif

void Renderer::RenderBackground() {
    ::RenderBackground(draw_list, window, viewport);
}
void Renderer::DrawObject(const core::Point &p) {
    const float half = 1.0f;
    draw_list->AddRectFilled(ImVec2(p.x - half, p.y - half),
                             ImVec2(p.x + half, p.y + half),
                             p.object_color, 2.0f, ImDrawFlags_RoundCornersAll);
}

void Renderer::DrawObject(const core::Line &line) {
    const float width = 2.0f;
    draw_list->AddLine(ToImVec2(line.a), ToImVec2(line.b), line.object_color, width);
}

void Renderer::DrawObject(const core::Wireframe &wireframe) {
    const float width = 2.0f;
    int size = wireframe.points.size();
    for (int i = 0; i < size-1; i++) {
        draw_list->AddLine(ToImVec2(wireframe.points[i]), ToImVec2(wireframe.points[i+1]), wireframe.object_color, width);
    }
}

/* Polyogon drawing helper functions*/
float polygonArea(const std::vector <ImVec2>& p) {
    float A = 0;
    for (int i = 0; i < (int)p.size(); i++) {
        int j = (i + 1) % p.size();
        A += p[i].x * p[j].y - p[j].x * p[i].y;
    }
    return A * 0.5f;
}

bool isCCW(const std::vector <ImVec2>& p) {
    return polygonArea(p) > 0;
}

float cross(const ImVec2& a, const ImVec2& b, const ImVec2& c) {
    return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
}

bool isConvex(const ImVec2& prev, const ImVec2& curr, const ImVec2& next, bool ccw) {
    float c = cross(prev, curr, next);
    return ccw ? (c > 0) : (c < 0);
}

bool pointInTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p) {
    float c1 = cross(a, b, p);
    float c2 = cross(b, c, p);
    float c3 = cross(c, a, p);

    // strictly inside (avoids edge issues)
    return (c1 > 0 && c2 > 0 && c3 > 0) ||
           (c1 < 0 && c2 < 0 && c3 < 0);
}

std::vector<int> Renderer::triangulate(std::vector <ImVec2> poly) {
    std::vector<int> result;
    int n = poly.size();
    if (n < 3) return result;

    bool ccw = isCCW(poly);

    std::vector<int> V(n);
    for (int i = 0; i < n; i++) V[i] = i;

    while (V.size() > 3) {
        bool ear_found = false;

        for (int i = 0; i < (int)V.size(); i++) {
            int prev = V[(i - 1 + V.size()) % V.size()];
            int curr = V[i];
            int next = V[(i + 1) % V.size()];

            if (!isConvex(poly[prev], poly[curr], poly[next], ccw))
                continue;

            bool inside = false;
            for (int j = 0; j < (int)V.size(); j++) {
                int vi = V[j];
                if (vi == prev || vi == curr || vi == next)
                    continue;

                if (pointInTriangle(poly[prev], poly[curr], poly[next], poly[vi])) {
                    inside = true;
                    break;
                }
            }
            if (inside) continue;
            // EAR FOUND
            result.push_back(prev);
            result.push_back(curr);
            result.push_back(next);

            V.erase(V.begin() + i);
            ear_found = true;
            break;
        }

        if (!ear_found) {
            // This means polygon is probably invalid (self-intersecting)
            break;
        }
    }

    if (V.size() == 3) {
        result.push_back(V[0]);
        result.push_back(V[1]);
        result.push_back(V[2]);
    }

    return result;
}

void Renderer::DrawObject(const core::Polygon &polygon) {
    const float width = 2.0f;
    int size = polygon.points.size();
    for (int i = 0; i < size; i++) {
        const core::Point& p1 = polygon.points[i];
        const core::Point& p2 = polygon.points[(i + 1) % size];
        draw_list->AddLine(ToImVec2(p1), ToImVec2(p2), polygon.object_color, width);
    }
    if (polygon.filled) {
        std::vector<ImVec2> vertices;
        for (const auto& p : polygon.points) {
            vertices.push_back(ToImVec2(p)); 
        }
        auto tris = triangulate(vertices);
        for (int i = 0; i < (int)tris.size(); i += 3) {
            draw_list->AddTriangleFilled(
                ImVec2(vertices[tris[i]].x,     vertices[tris[i]].y),
                ImVec2(vertices[tris[i+1]].x,   vertices[tris[i+1]].y),
                ImVec2(vertices[tris[i+2]].x,   vertices[tris[i+2]].y),
                polygon.object_color
            );
        }
        //draw_list->AddConvexPolyFilled(vertices.Data, vertices.Size, polygon.object_color);
    }
}

// Redundante. Método igual ao Draw do wireframe.
// Por algum motivo não está renderizando
void Renderer::DrawObject(const core::Curve2D &Curve2D) {
    const float width = 2.0f;
    int size = Curve2D.points.size();
    for (int i = 0; i < size-1; i++) {
        draw_list->AddLine(ToImVec2(Curve2D.points[i]), ToImVec2(Curve2D.points[i+1]), Curve2D.object_color, width);
    }
}

/* ======================== DEPRECATED ============================
// Esse método manipula diretamente os ponteiros da drawlist para realizar a escrita em paralelo (A drawlist não possui mecanismos de acesso concorrentes).
// Só é utilizado na rotina make fast, e não está funcionando atualmente (falta implementar preenchimento do poligono aqui).
void Renderer::DrawAllParallel() {
    const int n_points = (int)drawPointList.size();
    const int n_lines  = (int)drawLineList.size();
    const int n_wlines = (int)drawWireframeList.size();
    const int n_all_lines = n_lines + n_wlines;

    // Cada ponto vira um quad preenchido: 4 vértices, 6 índices (2 triângulos).
    // Cada segmento de reta vira um quad espesso: mesma conta.
    constexpr int VPP = 4, IPP = 6;   // vértices/índices por ponto
    constexpr int VPL = 4, IPL = 6;   // vértices/índices por linha
    constexpr float HP = 2.0f;        // meia-largura do quad de ponto (pixels)
    constexpr float HL = 1.0f;        // meia-espessura da linha (pixels)

    const int total_vtx = n_points * VPP + n_all_lines * VPL;
    const int total_idx = n_points * IPP + n_all_lines * IPL;
    if (total_vtx == 0) return;

    // UV de pixel branco — necessário para desenhar cor sólida via ImDrawList.
    const ImVec2 uv = ImGui::GetFontTexUvWhitePixel();

    // === SERIAL: reserva todo o espaço de uma vez ===
    // PrimReserve redimensiona VtxBuffer e IdxBuffer e aponta _VtxWritePtr/_IdxWritePtr
    // para o início da região recém-alocada. _VtxCurrentIdx NÃO é alterado aqui.
    const unsigned int vtx_base = draw_list->_VtxCurrentIdx;
    draw_list->PrimReserve(total_idx, total_vtx);
    ImDrawVert* const vtx_ptr = draw_list->_VtxWritePtr;
    ImDrawIdx*  const idx_ptr = draw_list->_IdxWritePtr;

    // === PARALLEL: preenche pontos como quads preenchidos ===
    // Cada thread escreve numa fatia não-sobrepostas de vtx_ptr/idx_ptr.
    // O índice i é derivado do ponteiro para evitar alocação de vetor de índices.
    if (n_points > 0) {
        cg_parallel_for_each(drawPointList.begin(), drawPointList.end(), [&](const core::Point& p) {
            const int i = (int)(&p - drawPointList.data());
            const unsigned int v0 = vtx_base + i * VPP;
            ImDrawVert* v  = vtx_ptr + i * VPP;
            ImDrawIdx*  ix = idx_ptr + i * IPP;
            #ifdef DONT_USE_OBJECT_COLOR
                const ImU32 col = IM_COL32_WHITE;
            #else
                const ImU32 col = (ImU32)p.object_color;
            #endif
            v[0] = { {p.x - HP, p.y - HP}, uv, col };
            v[1] = { {p.x + HP, p.y - HP}, uv, col };
            v[2] = { {p.x + HP, p.y + HP}, uv, col };
            v[3] = { {p.x - HP, p.y + HP}, uv, col };
            ix[0] = v0;     ix[1] = v0 + 1; ix[2] = v0 + 2;
            ix[3] = v0;     ix[4] = v0 + 2; ix[5] = v0 + 3;
        });
    }

    // === PARALLEL: preenche segmentos de reta como quads espessos ===
    // Calcula o vetor perpendicular normalizado e extrudamos os dois endpoints.
    // drawLineList e drawWireframeList são processados como um único espaço contíguo
    // no buffer (drawLineList vem primeiro, wireframeList logo depois).
    auto fill_line_batch = [&](const std::vector<core::Line>& lines, int batch_offset) {
        if (lines.empty()) return;
        const int base_vtx = n_points * VPP + batch_offset * VPL;
        const int base_idx = n_points * IPP + batch_offset * IPL;
        cg_parallel_for_each(lines.begin(), lines.end(), [&](const core::Line& l) {
            const int i = (int)(&l - lines.data());
            const unsigned int v0 = vtx_base + base_vtx + i * VPL;
            ImDrawVert* v  = vtx_ptr + base_vtx + i * VPL;
            ImDrawIdx*  ix = idx_ptr + base_idx + i * IPL;
            #ifdef DONT_USE_OBJECT_COLOR
                const ImU32 col = IM_COL32_WHITE;
            #else
                const ImU32 col = (ImU32)l.object_color;
            #endif
            // Vetor perpendicular escalado para meia-espessura
            const float dx = l.b.x - l.a.x;
            const float dy = l.b.y - l.a.y;
            const float len = std::sqrt(dx * dx + dy * dy);
            float nx = 0.0f, ny = HL;
            if (len > 1e-6f) { nx = (-dy / len) * HL; ny = (dx / len) * HL; }

            v[0] = { {l.a.x - nx, l.a.y - ny}, uv, col };
            v[1] = { {l.a.x + nx, l.a.y + ny}, uv, col };
            v[2] = { {l.b.x + nx, l.b.y + ny}, uv, col };
            v[3] = { {l.b.x - nx, l.b.y - ny}, uv, col };
            ix[0] = v0;     ix[1] = v0 + 1; ix[2] = v0 + 2;
            ix[3] = v0;     ix[4] = v0 + 2; ix[5] = v0 + 3;
        });
    };

    fill_line_batch(drawLineList, 0);
    fill_line_batch(drawWireframeList, n_lines);

    // === SERIAL: avança os ponteiros de escrita do ImDrawList ===
    // Após o preenchimento paralelo os ponteiros ainda apontam para o início
    // da região reservada; precisamos movê-los para depois do que escrevemos.
    draw_list->_VtxWritePtr   += total_vtx;
    draw_list->_IdxWritePtr   += total_idx;
    draw_list->_VtxCurrentIdx += total_vtx;
}
    ======================== DEPRECATED ============================ */

void Renderer::DrawPreview() {
    const auto& pts = displayFile.getPreviewPoints();
    if (pts.empty()) return;

    core::ShapeType mode = displayFile.getPreviewMode();
    if (mode == core::ShapeType::POINT) return;

    auto ncs_mat = window.GetWindowNCSMatrix();
    ImVec2 offset = viewport.GetCanvasP().first;

    switch (mode) {
        case core::ShapeType::LINE: // mesmo do wireframe
        case core::ShapeType::WIREFRAME:
            DrawPreviewPolyline(draw_list, pts, ncs_mat, window, offset); break;
        case core::ShapeType::POLYGON:
            DrawPreviewPolygon(draw_list, pts, ncs_mat, window, offset);  break;
        case core::ShapeType::CURVE2D: {
            int method = displayFile.getPreviewMethod();
            switch(method){
                case 0: DrawPreviewCurve2DBezier(draw_list, pts, ncs_mat, window, offset); break;
                case 1: DrawPreviewCurve2DBSpline(draw_list, pts, ncs_mat, window, offset); break;
                default: DrawPreviewCurve2DBezier(draw_list, pts, ncs_mat, window, offset); break;
            }
        }
        default: break;
    }
}

void Renderer::ApplyClipping(){
    core::Point ncs_min(-1.0f, -1.0f, 0.0f);
    core::Point ncs_max(1.0f, 1.0f, 0.0f);
    
    this->drawPointList = ClipPoints(this->drawPointList, ncs_min, ncs_max);
    this->drawLineList = ClipLines(this->drawLineList, ncs_min, ncs_max, viewport.GetClippingMode());
    this->drawWireframeList = ClipWireframes(this->wireframeMiddleware, ncs_min, ncs_max);
    this->drawPolygonList = ClipPolygons(this->drawPolygonList, ncs_min, ncs_max);
    // To test point clipping (método descrito), swap to: ClipCurve2DsByPoint(...)
    this->drawCurve2DList = ClipCurve2DsByPoint(this->Curve2DMiddleware, ncs_min, ncs_max);
}

void Renderer::ApplyNCSTransform(){
    this->drawPointList = displayFile.getPointList();
    this->drawLineList  = displayFile.getLineList();
    this->wireframeMiddleware = displayFile.getWireframeList();
    this->drawPolygonList = displayFile.getPolygonList();
    this->Curve2DMiddleware = displayFile.getCurve2DList();
    
    auto ncs_mat = window.GetWindowNCSMatrix();
    
    TransformToNCS(this->drawPointList, ncs_mat);
    TransformToNCS(this->drawLineList, ncs_mat);
    TransformToNCS(this->wireframeMiddleware, ncs_mat);
    TransformToNCS(this->drawPolygonList, ncs_mat);
    TransformToNCS(this->Curve2DMiddleware, ncs_mat);
}

void Renderer::ApplyViewportTransform(){
    auto canvas_p = viewport.GetCanvasP();
    ImVec2 offset = canvas_p.first;

    TransformToViewport(this->drawPointList, window, offset);
    TransformToViewport(this->drawLineList, window, offset);
    TransformToViewport(this->drawWireframeList, window, offset);
    TransformToViewport(this->drawPolygonList, window, offset);
    TransformToViewport(this->drawCurve2DList, window, offset);
}

void Renderer::GenerateDrawList(){
    unsigned long obj_count = displayFile.object_count;
    WindowAttributes w = window.getWindowAttributes();
    auto canvas_p = viewport.GetCanvasP();
    if(this->refresh_cache || rendererCache.cache_changed(w, obj_count, canvas_p.first, canvas_p.second)){
        log.AddLog("Scene changed, refreshing object cache\n");
        rendererCache.store_cache(w, obj_count, canvas_p.first, canvas_p.second);
        refresh_cache = false;

        ApplyNCSTransform(); 
        ApplyClipping();
        ApplyViewportTransform();
    }
}

void Renderer::render() {
    this->draw_list = viewport.GetDrawList();
    RenderBackground();
    GenerateDrawList();

    // #ifdef USE_PARALLEL_DRAWLIST
    //     DrawAllParallel();
    // #else
        for (const auto &p : drawPointList)    DrawObject(p);
        for (const auto &l : drawLineList)     DrawObject(l);
        for (const auto &w : drawWireframeList) DrawObject(w);
        for (const auto &p : drawPolygonList) DrawObject(p);
        for (const auto &b : drawCurve2DList) DrawObject(b);
    // #endif

    #ifndef DONT_DRAW_SHAPE_NAME
        for(const auto &p: displayFile.getPointList()) draw_name_if_visible(p);
        for(const auto &l: displayFile.getLineList()) draw_name_if_visible(l);
        for(const auto &w: displayFile.getWireframeList()) draw_name_if_visible(w);
        for(const auto &p: displayFile.getPolygonList()) draw_name_if_visible(p);
        for(const auto &b: displayFile.getCurve2DList()) draw_name_if_visible(b);
    #endif

    DrawPreview();
}

void Renderer::notifyTransformation(){
    this->refresh_cache = true;
}
