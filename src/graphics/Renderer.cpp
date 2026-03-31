#include "Renderer.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

inline ImVec2 ToImVec2(const core::Point &p) {
    return ImVec2(p.x, p.y);
}

void Renderer::renderName(const core::Shape &shape){
    
    #ifdef DRAW_SHAPE_NAME
        core::Point anchor = core::Point(shape.anchorPoint());
        DrawObject(anchor);
        core::Point p = window.WorldToViewport(anchor);
        const int magic_number = 15;
        ImVec2 pos(p.x, p.y - magic_number);

        draw_list->AddText(pos, IM_COL32_WHITE, shape.name.c_str());

    #endif
}

// Isso aqui calcula o espaçamento entre as linhas da grid de forma similar ao geogebra
float calculate_step(float world_width){
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

void Renderer::DrawObject(const core::Point &world_p) {
    core::Point screen_p = window.WorldToViewport(world_p);

    const float rad = 2.5;
    draw_list->AddCircle(ToImVec2(screen_p), rad, IM_COL32_WHITE, 0, 2.0f);
}

void Renderer::DrawObject(const core::Line &line) {
    core::Point p0 = window.WorldToViewport(line.a); 
    core::Point p1 = window.WorldToViewport(line.b);

    const float width = 2.0f;
    draw_list->AddLine(ToImVec2(p0), ToImVec2(p1), IM_COL32_WHITE, width);
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

void Renderer::render() {
    this->draw_list = viewport.GetDrawList();

    RenderBackground();
    for (const core::Point &point: displayFile.getPointList()) {
        DrawObject(point);
        renderName(point);
    }
    for (const core::Line &line: displayFile.getLineList()) {
        DrawObject(line);
        renderName(line);
    }
    for (const core::Wireframe &wireframe: displayFile.getWireframeList()) {
        DrawObject(wireframe);
        renderName(wireframe);
    }
    log.Draw("Log");
}