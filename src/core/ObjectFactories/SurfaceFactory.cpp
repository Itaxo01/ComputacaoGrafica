#include "SurfaceFactory.hpp"

namespace core {

// Basis matrices M[row][col], row = power of the parameter (t³, t², t, 1).
// Q(t) = T·M·G with T = [t³ t² t 1].
static const float M_BEZIER[4][4] = {
    {-1.0f,  3.0f, -3.0f, 1.0f},
    { 3.0f, -6.0f,  3.0f, 0.0f},
    {-3.0f,  3.0f,  0.0f, 0.0f},
    { 1.0f,  0.0f,  0.0f, 0.0f},
};
static const float M_BSPLINE[4][4] = {
    {-1.0f/6, 3.0f/6, -3.0f/6, 1.0f/6},
    { 3.0f/6,-6.0f/6,  3.0f/6, 0.0f  },
    {-3.0f/6, 0.0f,    3.0f/6, 0.0f  },
    { 1.0f/6, 4.0f/6,  1.0f/6, 0.0f  },
};

// blend[j] = Σ_p param^pow[p] · M[p][j], with param vector [u³ u² u 1].
static void blend(const float M[4][4], float u, float out[4]) {
    const float pv[4] = { u*u*u, u*u, u, 1.0f };
    for (int j = 0; j < 4; ++j) {
        float acc = 0.0f;
        for (int p = 0; p < 4; ++p) acc += pv[p] * M[p][j];
        out[j] = acc;
    }
}

// One patch point: P(s,t) = Σ_i Σ_j bs[i]·bt[j]·G[i][j], G row-major (i*4+j).
static core::Point evalPatch(const std::vector<core::Point>& cp,
                             const float bs[4], const float bt[4]) {
    core::Point acc(0, 0, 0);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            acc = acc + cp[i * 4 + j] * (bs[i] * bt[j]);
    return acc;
}

SurfaceFactory::SurfaceFactory(const std::string& name,
                               const std::vector<std::vector<core::Point>>& patches,
                               int method, int resolution, ImU32 color)
    : name_(name), patches_(patches), method_(method),
      resolution_(resolution < 2 ? 2 : resolution), color_(color) {}

Object SurfaceFactory::build() {
    Object obj;
    obj.name = name_;
    obj.type = ObjectType::SURFACE;
    obj.material.color = color_;

    const float (*M)[4] = (method_ == BSPLINE) ? M_BSPLINE : M_BEZIER;
    const int R = resolution_;
    core::Mesh& mesh = *obj.mesh;

    for (const auto& cp : patches_) {
        if (cp.size() < 16) continue;            // skip malformed patch
        const uint32_t base = (uint32_t)mesh.vertices.size();

        // Sample an R x R grid: index(si, ti) = base + si*R + ti.
        for (int si = 0; si < R; ++si) {
            float s = (float)si / (float)(R - 1);
            float bs[4]; blend(M, s, bs);
            for (int ti = 0; ti < R; ++ti) {
                float t = (float)ti / (float)(R - 1);
                float bt[4]; blend(M, t, bt);
                mesh.vertices.push_back(evalPatch(cp, bs, bt));
            }
        }

        // Grid lines: iso-s curves (vary t) and iso-t curves (vary s).
        for (int si = 0; si < R; ++si)
            for (int ti = 0; ti + 1 < R; ++ti)
                mesh.line_indices.push_back({ base + si * R + ti, base + si * R + ti + 1 });
        for (int ti = 0; ti < R; ++ti)
            for (int si = 0; si + 1 < R; ++si)
                mesh.line_indices.push_back({ base + si * R + ti, base + (si + 1) * R + ti });
    }

    meta_.patches    = patches_;
    meta_.method     = method_;
    meta_.resolution = resolution_;
    return obj;
}

std::unique_ptr<Metadata> SurfaceFactory::takeMetadata() {
    return std::make_unique<SurfaceMetadata>(std::move(meta_));
}

} // namespace core
