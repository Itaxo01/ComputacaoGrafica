#include "Renderer.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include "RendererUtils.hpp"

inline ImVec2 ToImVec2(const core::Point &p) {
    return ImVec2(p.x, p.y);
}

void Renderer::renderName(const core::Shape &shape){
    
    #ifndef DONT_DRAW_SHAPE_NAME
        core::Point p = core::Point(shape.anchorPoint());
        DrawObject(p);
        const int magic_number = 15;
        ImVec2 pos(p.x, p.y - magic_number);

        draw_list->AddText(pos, IM_COL32_WHITE, shape.name.c_str());

    #endif
}

// Isso aqui calcula o espaçamento entre as linhas da grid de forma similar ao geogebra
inline float calculate_step(float world_width){
    float raw_step = world_width/10.0f;
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

    core::Point world_min = window.GetWorldMin(); 
    core::Point world_max = window.GetWorldMax();
    core::Point origin_on_screen = window.WorldToViewport(core::Point(0, 0));
    
    
    // 3. Render Grid 
    if (viewport.show_grid) { 
        float world_width = world_max.x - world_min.x;
        float step = calculate_step(world_width);
        
        float text_y = std::clamp(origin_on_screen.y, canvas_p0.y, canvas_p1.y - 15.0f);
        float text_x = std::clamp(origin_on_screen.x, canvas_p0.x, canvas_p1.x - 30.0f);

        char label[32];
        // Vertical grid lines
        for (float x = std::floor(world_min.x / step) * step; x <= world_max.x; x += step) {
            if(x < world_min.x) continue;
            core::Point top_screen = window.WorldToViewport(core::Point(x, world_max.y));
            core::Point bottom_screen = window.WorldToViewport(core::Point(x, world_min.y));
            draw_list->AddLine(ToImVec2(top_screen), ToImVec2(bottom_screen), IM_COL32(100, 100, 100, 255));
            
            if (viewport.show_axis_coordinates && std::abs(x) > step * 0.1f) {
                snprintf(label, sizeof(label), "%g", x);  // %g automatically removes trailing bounds
                draw_list->AddText(ImVec2(top_screen.x + 4, text_y + 4), IM_COL32(200, 200, 200, 255), label);
            }
        }

        // Horizontal grid lines
        for (float y = std::floor(world_min.y / step) * step; y <= world_max.y; y += step) {
            if(y < world_min.y) continue;
            core::Point rightmost = window.WorldToViewport(core::Point(world_max.x, y));
            core::Point leftmost = window.WorldToViewport(core::Point(world_min.x, y));
            draw_list->AddLine(ToImVec2(rightmost), ToImVec2(leftmost), IM_COL32(100, 100, 100, 255));
            
            if (viewport.show_axis_coordinates && std::abs(y) > step * 0.1f) {
                snprintf(label, sizeof(label), "%g", y);
                // Offset slightly left of the vertical bounds
                draw_list->AddText(ImVec2(text_x + 4, rightmost.y - 16), IM_COL32(200, 200, 200, 255), label);
            }
        }
        if (viewport.show_axis_coordinates) {
                draw_list->AddText(ImVec2(text_x - 12, text_y + 4), IM_COL32(200, 200, 200, 255), "0");
        }
    }

    // 4. Render Axes (World X=0 and Y=0)
    if (viewport.show_axes) {
        // desenha somente se for visivel
        if (origin_on_screen.x >= canvas_p.first.x && origin_on_screen.x <= canvas_p.second.x) {
            draw_list->AddLine(ImVec2(origin_on_screen.x, canvas_p.first.y), 
                               ImVec2(origin_on_screen.x, canvas_p.second.y), 
                               IM_COL32(100, 100, 100, 255), 2.0f); // Green Y-axis
        }

        if (origin_on_screen.y >= canvas_p.first.y && origin_on_screen.y <= canvas_p.second.y) {
            draw_list->AddLine(ImVec2(canvas_p.first.x, origin_on_screen.y), 
                               ImVec2(canvas_p.second.x, origin_on_screen.y), 
                               IM_COL32(100, 100, 100, 255), 2.0f); // Red X-axis
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
    //draw_list->PopClipRect(); // ver oq faz
}

void Renderer::DrawObject(const core::Wireframe &wireframe) {
    const float width = 2.0f;
    int size = wireframe.points.size();
    for (int i = 0; i < size-1; i++) {
        core::Point p0 = window.WorldToViewport(wireframe.points[i]); 
        core::Point p1 = window.WorldToViewport(wireframe.points[i+1]);

        draw_list->AddLine(ToImVec2(p0), ToImVec2(p1), IM_COL32_WHITE, width);
    }
}


void Renderer::ApplyClipping(const core::Point &wp0, const core::Point &wp1){
    // ainda falta um para Polígono (filled)
    this->drawPointList = ClipPoints(displayFile.getPointList(), wp0, wp1);
    this->drawLineList = ClipLines(displayFile.getLineList(), wp0, wp1);
    this->drawWireframeList = ClipWireframes(displayFile.getWireframeList(), wp0, wp1);
}

/* Aplicamos a transformação no próprio vetor para máximo paralelismo */
void Renderer::ApplyViewportTransform(){
    ViewportTransform(drawPointList, window);
    ViewportTransform(drawLineList, window);
    ViewportTransform(drawWireframeList, window);
}

/* Esse método pega os objetos do DisplayFile e aplica os cálculos necessários:
    - Clipping
    - Viewport Transform
@ Após isso, a lista de pontos interna do Renderer já deverá estar formatada para a exibição (Resta apenas colocar no draw_list).
*/
void Renderer::GenerateDrawList(){
    core::Point wp0 = window.GetWorldMin(), wp1 = window.GetWorldMax();
    unsigned long obj_count = displayFile.object_count;
    
    if(rendererCache.cache_changed(wp0, wp1, obj_count)){
        rendererCache.store_cache(wp0, wp1, obj_count);
        ApplyClipping(wp0, wp1);
        ApplyViewportTransform();
    }
}

void Renderer::render() {
    this->draw_list = viewport.GetDrawList();
    GenerateDrawList();    
    RenderBackground();

    for (const core::Point &point: drawPointList) DrawObject(point);
    for (const core::Line &line: drawLineList) DrawObject(line);
    for(const core::Line &w_line: drawWireframeList) DrawObject(w_line);
    

    #ifndef DONT_DRAW_SHAPE_NAME // Lidamos com os nomes separadamente
        core::Point wp0 = window.GetWorldMin();
        core::Point wp1 = window.GetWorldMax();
        
        auto draw_name_if_visible = [&](const core::Shape& shape){
            core::Point anchor = shape.anchorPoint();
            
            if (anchor.x >= wp0.x && anchor.x <= wp1.x && 
                anchor.y >= wp0.y && anchor.y <= wp1.y) {
                
                core::Point p = window.WorldToViewport(anchor);
                const int magic_number = 15;
                ImVec2 pos(p.x, p.y - magic_number);

                draw_list->AddText(pos, IM_COL32_WHITE, shape.name.c_str());
            }
        };
        for(const auto &p: displayFile.getPointList()) draw_name_if_visible(p);
        for(const auto &l: displayFile.getLineList()) draw_name_if_visible(l);
        for(const auto &w: displayFile.getWireframeList()) draw_name_if_visible(w);
    #endif
    
    log.Draw("Log");
}