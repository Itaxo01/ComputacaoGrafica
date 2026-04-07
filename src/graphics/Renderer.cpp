#include "Renderer.hpp"
#include "Window.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include "RendererUtils.hpp"

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

    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

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
    const float rad = 2.5;
    draw_list->AddCircle(ToImVec2(p), rad, IM_COL32_WHITE, 0, 2.0f);
}

void Renderer::DrawObject(const core::Line &line) {
    const float width = 2.0f;
    draw_list->AddLine(ToImVec2(line.a), ToImVec2(line.b), IM_COL32_WHITE, width);
}

void Renderer::DrawObject(const core::Wireframe &wireframe) {
    const float width = 2.0f;
    int size = wireframe.points.size();
    for (int i = 0; i < size-1; i++) {
        core::Point p0 = window.WindowToViewport(wireframe.points[i]); 
        core::Point p1 = window.WindowToViewport(wireframe.points[i+1]);

        draw_list->AddLine(ToImVec2(p0), ToImVec2(p1), IM_COL32_WHITE, width);
    }
}


void Renderer::ApplyClipping(){
    core::Point ncs_min(-1.0f, -1.0f, 0.0f);
    core::Point ncs_max(1.0f, 1.0f, 0.0f);
    
    this->drawPointList = ClipPoints(this->drawPointList, ncs_min, ncs_max);
    this->drawLineList = ClipLines(this->drawLineList, ncs_min, ncs_max);
    this->drawWireframeList = ClipWireframes(this->wireframeMiddleware, ncs_min, ncs_max);
}

void Renderer::ApplyNCSTransform(){
    this->drawPointList = displayFile.getPointList();
    this->drawLineList  = displayFile.getLineList();
    this->wireframeMiddleware = displayFile.getWireframeList();
    
    auto ncs_mat = window.GetWindowNCSMatrix();
    
    TransformToNCS(this->drawPointList, ncs_mat);
    TransformToNCS(this->drawLineList, ncs_mat);
    TransformToNCS(this->wireframeMiddleware, ncs_mat);
}

void Renderer::ApplyViewportTransform(){
    auto canvas_p = viewport.GetCanvasP();
    ImVec2 offset = canvas_p.first;

    TransformToViewport(this->drawPointList, window, offset);
    TransformToViewport(this->drawLineList, window, offset);
    TransformToViewport(this->drawWireframeList, window, offset);
}

void Renderer::GenerateDrawList(){
    unsigned long obj_count = displayFile.object_count;
    WindowAttributes w = window.getWindowAttributes();
    if(this->refresh_cache || rendererCache.cache_changed(w, obj_count)){
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

    for (const core::Point &point: drawPointList) DrawObject(point);
    for (const core::Line &line: drawLineList) DrawObject(line);
    for (const core::Line &w_line: drawWireframeList) DrawObject(w_line);
    

    #ifndef DONT_DRAW_SHAPE_NAME 
        for(const auto &p: displayFile.getPointList()) draw_name_if_visible(p);
        for(const auto &l: displayFile.getLineList()) draw_name_if_visible(l);
        for(const auto &w: displayFile.getWireframeList()) draw_name_if_visible(w);
    #endif
    
    log.Draw("Log");
}

void Renderer::notifyTransformation(){
    this->refresh_cache = true;
}
