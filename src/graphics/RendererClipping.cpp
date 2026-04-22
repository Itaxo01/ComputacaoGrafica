#include "RendererClipping.hpp"
#include "ParallelUtils.hpp"
#include "Renderer.hpp"

// Helpers para o Cohen-Sutherland
typedef int OUT;
const int INSIDE = 0b0000;
const int LEFT   = 0b0001;
const int RIGHT  = 0b0010;
const int BOTTOM = 0b0100;
const int TOP    = 0b1000;

std::vector<core::Point> ClipPoints(const std::vector<core::Point> &v, const core::Point &wp0, const core::Point &wp1){
    std::vector<core::Point> ret(v.size());
    std::atomic<size_t> count{0}; // Utilizado para se livrar do mutex ao inserir no vetor
    
    cg_parallel_for_each(v.begin(), v.end(), [&](const core::Point &p){
        if(p.x >= wp0.x && p.x <= wp1.x && p.y >= wp0.y && p.y <= wp1.y){
            size_t insert_index = count.fetch_add(1, std::memory_order_relaxed);
            ret[insert_index] = p;
        }
    });
    ret.resize(count.load(std::memory_order_relaxed));
    return ret;
}

inline bool LineClipTest(float p, float q, float &u1, float &u2){
    if(p == 0.0f){ // A linha é paralela a algum limite da window
        if(q < 0.0) return false; // paralela e completamente fora da window.
    } else {
        float r = q/p;
        if (p < 0.0f) {
            if (r > u2) return false;
            if (r > u1) u1 = r;
        } else {
            if (r < u1) return false;
            if (r < u2) u2 = r;
        }
    }
    return true;
}

bool ClipLine(core::Line &line, const core::Point &wp0, const core::Point&wp1) {
    float u1 = 0.0f;
    float u2 = 1.0f;
    float dx = line.b.x - line.a.x;
    float dy = line.b.y - line.a.y;
    if(LineClipTest(-dx, line.a.x - wp0.x, u1, u2) && // left
        LineClipTest(dx, wp1.x - line.a.x, u1, u2)  && // right
        LineClipTest(-dy, line.a.y - wp0.y, u1, u2)  && // bottom
        LineClipTest(dy, wp1.y - line.a.y, u1, u2)     // top
    ){
        const float ox = line.a.x;
        const float oy = line.a.y;
        if(u1 > 0.0f){
            line.a.x = ox + u1 * dx;
            line.a.y = oy + u1 * dy;
        }
        if(u2 < 1.0f){
            line.b.x = ox + u2 * dx;
            line.b.y = oy + u2 * dy;
        }
        return true;
    }
    return false;
}

static inline bool ClipLineLiangBarsky(core::Line &line, const core::Point &wp0, const core::Point &wp1) {
    float u1 = 0.0f;
    float u2 = 1.0f;
    float dx = line.b.x - line.a.x;
    float dy = line.b.y - line.a.y;
    if(LineClipTest(-dx, line.a.x - wp0.x, u1, u2) && // left
        LineClipTest(dx, wp1.x - line.a.x, u1, u2)  && // right
        LineClipTest(-dy, line.a.y - wp0.y, u1, u2)  && // bottom
        LineClipTest(dy, wp1.y - line.a.y, u1, u2)     // top
    ){
        const float ox = line.a.x;
        const float oy = line.a.y;
        if(u1 > 0.0f){
            line.a.x = ox + u1 * dx;
            line.a.y = oy + u1 * dy;
        }
        if(u2 < 1.0f){
            line.b.x = ox + u2 * dx;
            line.b.y = oy + u2 * dy;
        }
        return true;
    }
    return false;
}

static inline OUT ComputeOut(const core::Point &p, const core::Point &wp0, const core::Point &wp1){
    OUT ret = INSIDE;
    if(p.x < wp0.x) ret |= LEFT;
    else if(p.x > wp1.x) ret |= RIGHT;
    if(p.y < wp0.y) ret |= BOTTOM;
    else if(p.y > wp1.y) ret |= TOP;
    return ret;
}

static inline bool ClipLineCohenSutherland(core::Line &line, const core::Point &wp0, const core::Point &wp1, std::vector<core::Line> &ret, std::atomic<size_t> &count) {
    // TODO
    OUT out1 = ComputeOut(line.a, wp0, wp1);
    OUT out2 = ComputeOut(line.b, wp0, wp1);
    bool accept = false;
    float &x0 = line.a.x;
    float &y0 = line.a.y;
    float &x1 = line.b.x;
    float &y1 = line.b.y;

	while (true) {
		if (!(out1 | out2)) { // bitwise OR is 0: both points inside window; trivially accept and exit loop
			accept = true;
			break;
		} else if (out1 & out2) {
            // bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
			// or BOTTOM), so both must be outside window; exit loop (accept is false)
			break;
		} else {
			// failed both tests, so calculate the line segment to clip
			// from an outside point to an intersection with clip edge
			float x = 0.0f, y = 0.0f;

			// At least one endpoint is outside the clip rectangle; pick it.
			OUT outcodeOut = out2 > out1 ? out2 : out1;

			// Now find the intersection point;
			// use formulas:
			//   slope = (y1 - y0) / (x1 - x0)
			//   x = x0 + (1 / slope) * (ym - y0), where ym is wp0.y or wp1.y
			//   y = y0 + slope * (xm - x0), where xm is xmin or wp1.x
			// No need to worry about divide-by-zero because, in each case, the
			// outcode bit being tested guarantees the denominator is non-zero
			if (outcodeOut & TOP) {           // point is above the clip window
				x = x0 + (x1 - x0) * (wp1.y - y0) / (y1 - y0);
				y = wp1.y;
			} else if (outcodeOut & BOTTOM) { // point is below the clip window
				x = x0 + (x1 - x0) * (wp0.y - y0) / (y1 - y0);
				y = wp0.y;
			} else if (outcodeOut & RIGHT) {  // point is to the right of clip window
				y = y0 + (y1 - y0) * (wp1.x - x0) / (x1 - x0);
				x = wp1.x;
			} else if (outcodeOut & LEFT) {   // point is to the left of clip window
				y = y0 + (y1 - y0) * (wp0.x - x0) / (x1 - x0);
				x = wp0.x;
			}

			// Now we move outside point to intersection point to clip
			// and get ready for next pass.
			if (outcodeOut == out1) {
				x0 = x;
				y0 = y;
				out1 = ComputeOut(line.a, wp0, wp1);
			} else {
				x1 = x;
				y1 = y;
				out2 = ComputeOut(line.b, wp0, wp1);
			}
		}
	}
	return accept;
}

std::vector<core::Line> ClipLines(const std::vector<core::Line> &v, const core::Point &wp0, const core::Point &wp1, int mode){
    std::vector<core::Line> ret(v.size());
    std::atomic<size_t> count{0};

    if (mode == 0) {
        cg_parallel_for_each(v.begin(), v.end(), [&](const core::Line &l){ 
            core::Line newline = l;
            if(ClipLineLiangBarsky(newline, wp0, wp1)){
                size_t insert_index = count.fetch_add(1, std::memory_order_relaxed);
                ret[insert_index] = newline;
            }
        });
    } else {
        cg_parallel_for_each(v.begin(), v.end(), [&](const core::Line &l){ 
            core::Line newline = l;
            if(ClipLineCohenSutherland(newline, wp0, wp1, ret, count)){
                size_t insert_index = count.fetch_add(1, std::memory_order_relaxed);
                ret[insert_index] = newline;
            }
        });
    }

    ret.resize(count.load(std::memory_order_relaxed));
    return ret;
}


std::vector<core::Line> ClipWireframes(const std::vector<core::Wireframe> &v, const core::Point &wp0, const core::Point &wp1){
    size_t max_lines = 0;
    for(const auto &w: v){
        if(w.points.size() > 1) {
            max_lines += w.points.size() - 1; // n pontos = n-1 linhas.
        }
    }

    std::vector<core::Line> ret(max_lines);
    std::atomic<size_t> count{0};

    cg_parallel_for_each(v.begin(), v.end(), [&](const core::Wireframe &w) {
        size_t p_count = w.points.size();
        if (p_count < 2) return; 

        // Quebramos o wireframe em segmentos de reta.
        for (size_t i = 0; i < p_count - 1; ++i) {
            core::Line line;
            line.a = w.points[i];
            line.b = w.points[i+1];
            
            // Reutilizamos o Liang-Barsky aqui. Poderia ser chamado ClipLines de novo, porém fazendo aqui permite popularmos o vetor diretamente.
            float u1 = 0.0f;
            float u2 = 1.0f;
            float dx = line.b.x - line.a.x;
            float dy = line.b.y - line.a.y;
            
            if(LineClipTest(-dx, line.a.x - wp0.x, u1, u2) && // left
               LineClipTest(dx, wp1.x - line.a.x, u1, u2)  && // right
               LineClipTest(-dy, line.a.y - wp0.y, u1, u2)  && // bottom
               LineClipTest(dy, wp1.y - line.a.y, u1, u2)     // top
            ){
                core::Line cLine = line;
                if(u1 > 0.0f){
                    cLine.a.x = line.a.x + u1 * dx;
                    cLine.a.y = line.a.y + u1 * dy;
                }
                if(u2 < 1.0f){
                    cLine.b.x = line.a.x + u2 * dx;
                    cLine.b.y = line.a.y + u2 * dy;
                }
                #ifndef DONT_USE_OBJECT_COLOR
                    cLine.object_color = w.object_color;
                #endif

                size_t insert_index = count.fetch_add(1, std::memory_order_relaxed);
                ret[insert_index] = cLine;
            }
        }
    });

    ret.resize(count.load(std::memory_order_relaxed));

    return ret;
}

// Método descrito no livro com rejeição trivial correta:
//   - ambos dentro  → desenha diretamente
//   - ambos fora no mesmo lado (rejeição trivial de Cohen-Sutherland) → descarta
//   - qualquer outro caso → aplica Liang-Barsky (cobre: um dentro/um fora,
//     e também ambos fora mas em lados opostos, onde o segmento pode atravessar a window)
std::vector<core::Line> ClipCurve2DsByPoint(const std::vector<core::Curve2D> &v, const core::Point &wp0, const core::Point &wp1) {
    std::vector<core::Line> ret;

    auto inside = [&](const core::Point &p) {
        return p.x >= wp0.x && p.x <= wp1.x && p.y >= wp0.y && p.y <= wp1.y;
    };

    for (const auto &curve : v) {
        const auto &pts = curve.points;
        if (pts.size() < 2) continue;

        for (size_t i = 0; i < pts.size() - 1; ++i) {
            bool a_in = inside(pts[i]);
            bool b_in = inside(pts[i + 1]);

            if (a_in && b_in) {
                // Ambos dentro: emite diretamente sem cálculo extra
                core::Line line;
                line.a = pts[i];
                line.b = pts[i + 1];
                #ifndef DONT_USE_OBJECT_COLOR
                    line.object_color = curve.object_color;
                #endif
                ret.push_back(line);
            } else {
                // Rejeição trivial: ambos fora do mesmo lado da window
                OUT oa = ComputeOut(pts[i],     wp0, wp1);
                OUT ob = ComputeOut(pts[i + 1], wp0, wp1);
                if (oa & ob) continue; // mesmo lado → segmento completamente fora

                // Qualquer outro caso: aplica Liang-Barsky
                core::Line line;
                line.a = pts[i];
                line.b = pts[i + 1];
                #ifndef DONT_USE_OBJECT_COLOR
                    line.object_color = curve.object_color;
                #endif
                if (ClipLineLiangBarsky(line, wp0, wp1))
                    ret.push_back(line);
            }
        }
    }

    return ret;
}

static inline bool SHClipping(std::vector<core::Point> &newP, const core::Point &wp0, const core::Point &wp1){
    for (int edge = 0; edge < 4; ++edge) {
        std::vector<core::Point> inputList = newP;
        newP.clear();
        if (inputList.empty()) break;

        core::Point A, B;
        switch (edge) {
            case 0: A = {wp0.x, wp0.y}; B = {wp0.x, wp1.y}; break; // left
            case 1: A = {wp1.x, wp0.y}; B = {wp1.x, wp1.y}; break; // right
            case 2: A = {wp0.x, wp0.y}; B = {wp1.x, wp0.y}; break; // bottom
            case 3: A = {wp0.x, wp1.y}; B = {wp1.x, wp1.y}; break; // top
        }

        auto isInside = [&](const core::Point &pt) {
            switch (edge) {
                case 0: return pt.x >= wp0.x; // left
                case 1: return pt.x <= wp1.x; // right
                case 2: return pt.y >= wp0.y; // bottom
                case 3: return pt.y <= wp1.y; // top
            }
            return false;
        };

        for (size_t i = 0; i < inputList.size(); ++i) {
            const core::Point &current = inputList[i];
            const core::Point &prev = inputList[(i + inputList.size() - 1) % inputList.size()];

            bool currentInside = isInside(current);
            bool prevInside = isInside(prev);

            if (currentInside && !prevInside) {
                float t = ((A.x - prev.x) * (B.y - A.y) - (A.y - prev.y) * (B.x - A.x)) /
                        ((B.x - A.x) * (prev.y - current.y) - (B.y - A.y) * (prev.x - current.x));
                newP.push_back({prev.x + t * (current.x - prev.x), prev.y + t * (current.y - prev.y)});
            }
            if (currentInside) {
                newP.push_back(current);
            }
            if (!currentInside && prevInside) {
                float t = ((A.x - prev.x) * (B.y - A.y) - (A.y - prev.y) * (B.x - A.x)) /
                        ((B.x - A.x) * (prev.y - current.y) - (B.y - A.y) * (prev.x - current.x));
                newP.push_back({prev.x + t * (current.x - prev.x), prev.y + t * (current.y - prev.y)});
            }
        }
    }
    return true;
}

std::vector<core::Polygon> ClipPolygons(const std::vector<core::Polygon> &v, const core::Point &wp0, const core::Point &wp1) {
    std::vector<core::Polygon> ret;
    ret.reserve(v.size());
    for (auto &p: v) { 
        if(p.filled){
            std::vector<ImVec2> vertices;
            for (const auto& pl : p.points) {
                vertices.push_back(ImVec2(pl.x, pl.y)); 
            }
            auto tris = Renderer::triangulate(vertices);
            for (int i = 0; i < (int)tris.size(); i += 3) {
                core::Polygon tri = p;
                tri.points = {
                    p.points[tris[i]],
                    p.points[tris[i+1]],
                    p.points[tris[i+2]]
                };
                auto newPolygonPoints = tri.points;
                if(SHClipping(newPolygonPoints, wp0, wp1)){
                    core::Polygon &np = ret.emplace_back(newPolygonPoints, p.filled);
                    np.object_color = p.object_color; 
                }
            }
        } else {
            auto newPolygonPoints = p.points;
            if(SHClipping(newPolygonPoints, wp0, wp1)){
                core::Polygon &np = ret.emplace_back(newPolygonPoints, p.filled);
                np.object_color = p.object_color; 
            }
        }
    }
    return ret;
}