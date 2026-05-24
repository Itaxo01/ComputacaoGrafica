#include "Triangulate.hpp"

namespace core {

static float polygonArea(const std::vector<ImVec2>& p) {
    float A = 0;
    for (int i = 0; i < (int)p.size(); i++) {
        int j = (i + 1) % (int)p.size();
        A += p[i].x * p[j].y - p[j].x * p[i].y;
    }
    return A * 0.5f;
}

static float cross_tri(const ImVec2& a, const ImVec2& b, const ImVec2& c) {
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

static bool isConvex(const ImVec2& prev, const ImVec2& curr, const ImVec2& next, bool ccw) {
    float c = cross_tri(prev, curr, next);
    return ccw ? (c > 0) : (c < 0);
}

static bool pointInTriangle(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& p) {
    float c1 = cross_tri(a, b, p);
    float c2 = cross_tri(b, c, p);
    float c3 = cross_tri(c, a, p);
    return (c1>0 && c2>0 && c3>0) || (c1<0 && c2<0 && c3<0);
}

std::vector<int> triangulate(std::vector<ImVec2> poly) {
    std::vector<int> result;
    int n = (int)poly.size();
    if (n < 3) return result;

    bool ccw = polygonArea(poly) > 0;
    std::vector<int> V(n);
    for (int i = 0; i < n; i++) V[i] = i;

    while ((int)V.size() > 3) {
        bool ear_found = false;
        for (int i = 0; i < (int)V.size(); i++) {
            int prev = V[(i - 1 + V.size()) % V.size()];
            int curr = V[i];
            int next = V[(i + 1) % V.size()];

            if (!isConvex(poly[prev], poly[curr], poly[next], ccw)) continue;

            bool inside = false;
            for (int j = 0; j < (int)V.size(); j++) {
                int vi = V[j];
                if (vi == prev || vi == curr || vi == next) continue;
                if (pointInTriangle(poly[prev], poly[curr], poly[next], poly[vi])) {
                    inside = true; break;
                }
            }
            if (inside) continue;

            result.push_back(prev);
            result.push_back(curr);
            result.push_back(next);
            V.erase(V.begin() + i);
            ear_found = true;
            break;
        }
        if (!ear_found) break;
    }
    if ((int)V.size() == 3) {
        result.push_back(V[0]);
        result.push_back(V[1]);
        result.push_back(V[2]);
    }
    return result;
}

} // namespace core
