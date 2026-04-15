#include "Renderer.hpp"
#include "Shape.hpp"
#include "Window.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <string>
#include "RendererUtils.hpp"
#include "ParallelUtils.hpp"

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

// Isso aqui calcula o espaçamento entre as linhas da grid de forma similar ao geogebra
inline float calculate_step(float width){
    float raw_step = width/10.0f;
    float exp = std::floor(std::log10(raw_step));
    float mag = std::pow(10.0f, exp);
    float fraction = raw_step/mag;
    float multiplier = 1.0f;
    if(fraction >= 1.5f && fraction < 3.5f) multiplier = 2.0f;
    else if(fraction >= 3.5f && fraction < 7.5f) multiplier = 5.0f;
    else if(fraction >= 7.5) multiplier = 10.0f;

    return multiplier*mag;
}

// Draw border and background color, also now is responsable for the grid and axis
void Renderer::RenderBackground() {
    std::pair<ImVec2, ImVec2> canvas_p = viewport.GetCanvasP();
    ImVec2 canvas_p0 = canvas_p.first; ImVec2 canvas_p1 = canvas_p.second;

    const float offset = 0.5f;
    ImVec2 canvas_p0_offset(canvas_p0.x - offset, canvas_p0.y - offset);
    ImVec2 canvas_p1_offset(canvas_p1.x + offset, canvas_p1.y + offset);
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0_offset, canvas_p1_offset, IM_COL32(255, 255, 255, 255), 1.0f, offset);

    WindowAttributes w_attr = window.getWindowAttributes();
    core::Point origin_on_screen = window.WindowToViewport(core::Point(0, 0, 0));
    
    core::Point ncs_min(-1.0f, -1.0f, 0.0f);
    core::Point ncs_max(1.0f, 1.0f, 0.0f);
    auto ncs_mat = window.GetWindowNCSMatrix();
    
    // 3. Render Grid 
    if (viewport.show_grid) { 
        float step = calculate_step(w_attr.width);
        
        char label[32];
        
        // Helper para prender o texto na linha da grid visível e o mais próximo possível do eixo correspondente.
        auto get_clamped_text_pos = [&](const core::Point& target_world, const core::Point& seg_a_ncs, const core::Point& seg_b_ncs) {
            core::Point target_ncs = ncs_mat * target_world;
            core::Point dir = seg_b_ncs - seg_a_ncs;
            float len2 = dir.x * dir.x + dir.y * dir.y; 
            
            core::Point result_ncs = target_ncs;
            if (len2 > 1e-6f) { 
                core::Point pa = target_ncs - seg_a_ncs;
                float t = (pa.x * dir.x + pa.y * dir.y) / len2;
                // Prende às bordas da parte visível da linha na tela
                t = std::clamp(t, 0.001f, 0.999f);
                result_ncs = seg_a_ncs + (dir * t);
            }
            
            core::Point screen_pos = window.NCSToViewport(result_ncs);
            screen_pos.x += canvas_p0.x;
            screen_pos.y += canvas_p0.y;
            return screen_pos;
        };

        // Algumas mudanças foram necessárias para lidar com a rotação da window
        // com a window rotacionada como a 45º, a abertura de visão é maior em um eixo do que no outro
        float max_radius = std::sqrt(w_attr.width * w_attr.width + w_attr.height * w_attr.height) / 2.0f;
        
        float start_x = std::floor((w_attr.center.x - max_radius) / step) * step;
        float end_x = w_attr.center.x + max_radius;
        float start_y = std::floor((w_attr.center.y - max_radius) / step) * step;
        float end_y = w_attr.center.y + max_radius;

        // Vertical grid lines
        for (float x = start_x; x <= end_x; x += step) {
            core::Point pa(x, start_y); core::Point pb(x, end_y);
            core::Line l(pa, pb);
            l.a = ncs_mat * l.a; l.b = ncs_mat * l.b;
            auto [clipped, isOnScreen]  = ClipLine(l, ncs_min, ncs_max);
            
            if (!isOnScreen) continue;

            core::Point top_screen = window.NCSToViewport(clipped.b);
            core::Point bottom_screen = window.NCSToViewport(clipped.a);
            top_screen.x += canvas_p0.x; top_screen.y += canvas_p0.y;
            bottom_screen.x += canvas_p0.x; bottom_screen.y += canvas_p0.y;

            draw_list->AddLine(ToImVec2(top_screen), ToImVec2(bottom_screen), IM_COL32(100, 100, 100, 255));
            
            if (viewport.show_axis_coordinates && std::abs(x) > step * 0.1f) {
                snprintf(label, sizeof(label), "%g", x);  
                
                core::Point screen_pos = get_clamped_text_pos(core::Point(x, 0, 0), clipped.a, clipped.b);
                
                ImVec2 text_dr(screen_pos.x + 6, screen_pos.y + 4);
                // Leve clamp para que o texto não corte nas extremidades (offset do AddText)
                text_dr.x = std::clamp(text_dr.x, canvas_p0.x + 2.0f, canvas_p1.x - 30.0f);
                text_dr.y = std::clamp(text_dr.y, canvas_p0.y + 2.0f, canvas_p1.y - 15.0f);
                
                draw_list->AddText(text_dr, IM_COL32(200, 200, 200, 255), label);
            }
        }

        // Horizontal grid lines
        for (float y = start_y; y <= end_y; y += step) {
            core::Point pa(start_x, y); core::Point pb(end_x, y);
            core::Line l(pa, pb);
            l.a = ncs_mat * l.a; l.b = ncs_mat * l.b;
            auto [clipped, isOnScreen]  = ClipLine(l, ncs_min, ncs_max);
            
            if (!isOnScreen) continue;

            core::Point rightmost = window.NCSToViewport(clipped.b);
            core::Point leftmost = window.NCSToViewport(clipped.a);
            rightmost.x += canvas_p0.x; rightmost.y += canvas_p0.y;
            leftmost.x += canvas_p0.x; leftmost.y += canvas_p0.y;

            draw_list->AddLine(ToImVec2(rightmost), ToImVec2(leftmost), IM_COL32(100, 100, 100, 255));
            
            if (viewport.show_axis_coordinates && std::abs(y) > step * 0.1f) {
                snprintf(label, sizeof(label), "%g", y);
                
                core::Point screen_pos = get_clamped_text_pos(core::Point(0, y, 0), clipped.a, clipped.b);
                
                ImVec2 text_dr(screen_pos.x + 6, screen_pos.y - 16);
                text_dr.x = std::clamp(text_dr.x, canvas_p0.x + 2.0f, canvas_p1.x - 30.0f);
                text_dr.y = std::clamp(text_dr.y, canvas_p0.y + 15.0f, canvas_p1.y - 15.0f);

                draw_list->AddText(text_dr, IM_COL32(200, 200, 200, 255), label);
            }
        }
        if (viewport.show_axis_coordinates) {
                core::Point origin_ncs = ncs_mat * core::Point(0, 0, 0);
                if(origin_ncs.x >= -1.0f && origin_ncs.x <= 1.0f && origin_ncs.y >= -1.0f && origin_ncs.y <= 1.0f){
                    core::Point origin_screen = window.NCSToViewport(origin_ncs);
                    draw_list->AddText(ImVec2(origin_screen.x + canvas_p0.x - 12, origin_screen.y + canvas_p0.y + 4), IM_COL32(200, 200, 200, 255), "0");
                }
        }
    }

    // 4. Render Axes (World X=0 and Y=0)
    if (viewport.show_axes) {
        float max_radius = std::sqrt(w_attr.width * w_attr.width + w_attr.height * w_attr.height) / 2.0f;
        core::Point center = w_attr.center;
        
        // Y Axis
        core::Point pay(0, center.y + max_radius, 0); core::Point pby(0, center.y - max_radius, 0);
        core::Line l_y(pay, pby);
        l_y.a = ncs_mat * l_y.a; l_y.b = ncs_mat * l_y.b;
        auto [clipped_y, isOnScreen_y]  = ClipLine(l_y, ncs_min, ncs_max);

        if(isOnScreen_y){
            core::Point top_screen = window.NCSToViewport(clipped_y.a);
            core::Point bottom_screen = window.NCSToViewport(clipped_y.b);
            top_screen.x += canvas_p0.x; top_screen.y += canvas_p0.y;
            bottom_screen.x += canvas_p0.x; bottom_screen.y += canvas_p0.y;
            draw_list->AddLine(ToImVec2(top_screen), ToImVec2(bottom_screen), IM_COL32(100, 100, 100, 255), 2.0f);
        }

        // X Axis
        core::Point pax(center.x + max_radius, 0, 0); core::Point pbx(center.x - max_radius, 0, 0);
        core::Line l_x(pax, pbx);
        l_x.a = ncs_mat * l_x.a; l_x.b = ncs_mat * l_x.b;
        auto [clipped_x, isOnScreen_x]  = ClipLine(l_x, ncs_min, ncs_max);

        if(isOnScreen_x){
            core::Point rightmost = window.NCSToViewport(clipped_x.a);
            core::Point leftmost = window.NCSToViewport(clipped_x.b);
            rightmost.x += canvas_p0.x; rightmost.y += canvas_p0.y;
            leftmost.x += canvas_p0.x; leftmost.y += canvas_p0.y;
            draw_list->AddLine(ToImVec2(rightmost), ToImVec2(leftmost), IM_COL32(100, 100, 100, 255), 2.0f);
        }
    }
}
void Renderer::DrawObject(const core::Point &p) {
    const float half = 2.0f;
    draw_list->AddRectFilled(ImVec2(p.x - half, p.y - half),
                             ImVec2(p.x + half, p.y + half),
                             p.object_color, 0, 2.0f);
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

void Renderer::DrawObject(const core::Polygon &polygon) {
    const float width = 2.0f;
    int size = polygon.points.size();
    for (int i = 0; i < size; i++) {
        const core::Point& p1 = polygon.points[i];
        const core::Point& p2 = polygon.points[(i + 1) % size];
        draw_list->AddLine(ToImVec2(p1), ToImVec2(p2), polygon.object_color, width);
    }
    if (polygon.filled) {
        ImVector<ImVec2> vertices;
        for (const auto& p : polygon.points) {
            vertices.push_back(ToImVec2(p)); 
        }
        draw_list->AddConvexPolyFilled(vertices.Data, vertices.Size, polygon.object_color);
    }
}


// Esse método manipula diretamente os ponteiros da drawlist para realizar a escrita em paralelo (A drawlist não possui mecanismos de acesso concorrentes).
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

void Renderer::DrawPreview() {
    const auto& pts = displayFile.getPreviewPoints();
    if (pts.empty()) return;

    core::ShapeType mode = displayFile.getPreviewMode();
    if (mode == core::ShapeType::POINT) return;

    auto canvas_p = viewport.GetCanvasP();
    ImVec2 offset  = canvas_p.first;
    auto ncs_mat   = window.GetWindowNCSMatrix();

    // World → NCS → viewport → screen
    auto to_screen = [&](float wx, float wy) -> ImVec2 {
        core::Point ncs = ncs_mat * core::Point(wx, wy, 0.0f);
        core::Point vp  = window.NCSToViewport(ncs);
        return ImVec2(vp.x + offset.x, vp.y + offset.y);
    };

    constexpr ImU32 COL_EDGE   = IM_COL32(200, 200, 200, 200);
    constexpr ImU32 COL_RUBBER = IM_COL32(200, 200, 200, 120);
    constexpr ImU32 COL_CLOSE  = IM_COL32(100, 200, 255, 100);
    constexpr ImU32 COL_VERTEX = IM_COL32(255, 220,  80, 220);

    // Lines between placed vertices
    for (size_t i = 1; i < pts.size(); i++) {
        auto [x0, y0, z0] = pts[i - 1];
        auto [x1, y1, z1] = pts[i];
        draw_list->AddLine(to_screen(x0, y0), to_screen(x1, y1), COL_EDGE, 1.5f);
    }

    // Rubber-band from last vertex to mouse cursor
    ImVec2 mouse = ImGui::GetMousePos();
    {
        auto [lx, ly, lz] = pts.back();
        draw_list->AddLine(to_screen(lx, ly), mouse, COL_RUBBER, 1.0f);
    }

    // Polygon closing hint: mouse → first vertex
    if (mode == core::ShapeType::POLYGON && pts.size() >= 2) {
        auto [fx, fy, fz] = pts.front();
        draw_list->AddLine(mouse, to_screen(fx, fy), COL_CLOSE, 1.0f);
    }

    // Vertex dots
    for (const auto& [px, py, pz] : pts) {
        draw_list->AddCircleFilled(to_screen(px, py), 3.5f, COL_VERTEX);
    }
}

void Renderer::ApplyClipping(){
    core::Point ncs_min(-1.0f, -1.0f, 0.0f);
    core::Point ncs_max(1.0f, 1.0f, 0.0f);
    
    this->drawPointList = ClipPoints(this->drawPointList, ncs_min, ncs_max);
    this->drawLineList = ClipLines(this->drawLineList, ncs_min, ncs_max, viewport.GetClippingMode());
    this->drawWireframeList = ClipWireframes(this->wireframeMiddleware, ncs_min, ncs_max);
    this->drawPolygonList = ClipPolygons(this->drawPolygonList, ncs_min, ncs_max);
}

void Renderer::ApplyNCSTransform(){
    this->drawPointList = displayFile.getPointList();
    this->drawLineList  = displayFile.getLineList();
    this->wireframeMiddleware = displayFile.getWireframeList();
    this->drawPolygonList = displayFile.getPolygonList();
    
    auto ncs_mat = window.GetWindowNCSMatrix();
    
    TransformToNCS(this->drawPointList, ncs_mat);
    TransformToNCS(this->drawLineList, ncs_mat);
    TransformToNCS(this->wireframeMiddleware, ncs_mat);
    TransformToNCS(this->drawPolygonList, ncs_mat);
}

void Renderer::ApplyViewportTransform(){
    auto canvas_p = viewport.GetCanvasP();
    ImVec2 offset = canvas_p.first;

    TransformToViewport(this->drawPointList, window, offset);
    TransformToViewport(this->drawLineList, window, offset);
    TransformToViewport(this->drawWireframeList, window, offset);
    TransformToViewport(this->drawPolygonList, window, offset);
}

void Renderer::GenerateDrawList(){
    unsigned long obj_count = displayFile.object_count;
    WindowAttributes w = window.getWindowAttributes();
    if(this->refresh_cache || rendererCache.cache_changed(w, obj_count)){
        log.AddLog("Scene changed, refreshing object cache\n");
        rendererCache.store_cache(w, obj_count);
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

    #ifdef USE_PARALLEL_DRAWLIST
        DrawAllParallel();
    #else
        for (const auto &p : drawPointList)    DrawObject(p);
        for (const auto &l : drawLineList)     DrawObject(l);
        for (const auto &w : drawWireframeList) DrawObject(w);
        for (const auto &p : drawPolygonList) DrawObject(p);
    #endif

    #ifndef DONT_DRAW_SHAPE_NAME
        for(const auto &p: displayFile.getPointList()) draw_name_if_visible(p);
        for(const auto &l: displayFile.getLineList()) draw_name_if_visible(l);
        for(const auto &w: displayFile.getWireframeList()) draw_name_if_visible(w);
        for(const auto &p: displayFile.getPolygonList()) draw_name_if_visible(p);
    #endif

    DrawPreview();

    log.Draw("Log");
}

void Renderer::notifyTransformation(){
    this->refresh_cache = true;
}
