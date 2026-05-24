#include "Curve2DFactory.hpp"

namespace core {

static core::Point lerp(const core::Point& p0, const core::Point& p1, float t) {
    return p0 + (p1 - p0) * t;
}

static std::vector<core::Point> construct_bezier(const std::vector<core::Point>& data, int steps) {
    std::vector<core::Point> result;
    int num_segments = ((int)data.size() - 1) / 3;
    for (int seg = 0; seg < num_segments; seg++) {
        core::Point p[4] = {data[seg*3], data[seg*3+1], data[seg*3+2], data[seg*3+3]};
        int start_i = (seg == 0) ? 0 : 1;
        for (int i = start_i; i < steps; i++) {
            float t = (float)i / (float)(steps - 1);
            core::Point q[4] = {p[0], p[1], p[2], p[3]};
            for (int d = 3; d > 0; d--)
                for (int j = 0; j < d; j++)
                    q[j] = lerp(q[j], q[j+1], t);
            result.push_back(q[0]);
        }
    }
    return result;
}

static std::vector<core::Point> construct_bspline(const std::vector<core::Point>& data, int steps) {
    std::vector<core::Point> result;
    if (data.size() < 4) return result;

    int num_segments = (int)data.size() - 3;
    float delta = 1.0f / (float)(steps - 1);
    float d2 = delta * delta, d3 = d2 * delta;

    for (int seg = 0; seg < num_segments; seg++) {
        core::Point p0 = data[seg], p1 = data[seg+1], p2 = data[seg+2], p3 = data[seg+3];
        core::Point A = (p0*-1.0f + p1* 3.0f + p2*-3.0f + p3*1.0f) * (1.0f/6.0f);
        core::Point B = (p0* 3.0f + p1*-6.0f + p2* 3.0f           ) * (1.0f/6.0f);
        core::Point C = (p0*-3.0f +             p2* 3.0f           ) * (1.0f/6.0f);
        core::Point D = (p0* 1.0f + p1* 4.0f + p2* 1.0f           ) * (1.0f/6.0f);

        core::Point f   = D;
        core::Point df  = A*d3 + B*d2 + C*delta;
        core::Point d2f = A*(6.0f*d3) + B*(2.0f*d2);
        core::Point d3f = A*(6.0f*d3);

        for (int i = 0; i < steps; i++) {
            if (seg == 0 || i > 0) result.push_back(f);
            f = f + df; df = df + d2f; d2f = d2f + d3f;
        }
    }
    return result;
}

Curve2DFactory::Curve2DFactory(const std::string& name, const std::vector<core::Point>& control_pts,
                               int smoothness, int method, ImU32 color)
    : name_(name), control_pts_(control_pts), smoothness_(smoothness),
      method_(method), color_(color) {}

Object Curve2DFactory::build() {
    Object obj;
    obj.name = name_;
    obj.type = ObjectType::CURVE2D;
    obj.material.color = color_;

    std::vector<core::Point> tessellated;
    switch (method_) {
        case BSPLINE: tessellated = construct_bspline(control_pts_, smoothness_); break;
        default:      tessellated = construct_bezier(control_pts_, smoothness_);  break;
    }

    obj.mesh->vertices = tessellated;
    for (uint32_t i = 0; i + 1 < (uint32_t)tessellated.size(); i++)
        obj.mesh->line_indices.push_back({i, i + 1});

    meta_.control_points = control_pts_;
    meta_.smoothness     = smoothness_;
    meta_.method         = method_;

    return obj;
}

std::unique_ptr<Metadata> Curve2DFactory::takeMetadata() {
    return std::make_unique<CurveMetadata>(std::move(meta_));
}

} // namespace core
