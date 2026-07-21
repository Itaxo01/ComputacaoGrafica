#include "SurfaceFactory.hpp"
#include <cstring>

namespace core {

// Basis matrices M[row][col], row = power of the parameter (t³, t², t, 1).
// A patch coordinate is Q(s,t) = S·M·G·Mᵀ·Tᵀ, S = [s³ s² s 1], T = [t³ t² t 1].
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

// out = A·B (4x4).
static void matMul(const float A[4][4], const float B[4][4], float out[4][4]) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += A[i][k] * B[k][j];
            out[i][j] = s;
        }
}

static void transpose(const float A[4][4], float out[4][4]) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) out[i][j] = A[j][i];
}

// Coefficient matrix C = M·G·Mᵀ for one coordinate (axis 0=x, 1=y, 2=z), where
// G is the 4x4 of that coordinate over the patch's control points (row-major).
static void coeffMatrix(const float M[4][4], const std::vector<core::Point>& cp,
                        int axis, float C[4][4]) {
    float G[4][4];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            const core::Point& p = cp[i * 4 + j];
            G[i][j] = (axis == 0) ? p.x : (axis == 1) ? p.y : p.z;
        }
    float MG[4][4], Mt[4][4];
    matMul(M, G, MG);
    transpose(M, Mt);
    matMul(MG, Mt, C);
}

// Forward-difference basis matrix E(δ): maps a cubic's coefficient vector
// [a³ a² a¹ a⁰] to its initial forward differences [f, Δf, Δ²f, Δ³f] at param 0
// with step δ. E·C·Eᵀ then gives the bidirectional forward differences of a patch.
static void forwardDiffE(float d, float E[4][4]) {
    float d2 = d * d, d3 = d2 * d;
    float tmp[4][4] = {
        {0.0f,     0.0f,     0.0f, 1.0f},
        {d3,       d2,       d,    0.0f},
        {6.0f*d3,  2.0f*d2,  0.0f, 0.0f},
        {6.0f*d3,  0.0f,     0.0f, 0.0f},
    };
    std::memcpy(E, tmp, sizeof(tmp));
}

// ── Blending-functions path (direct evaluation) ────────────────────────────────

// blend[j] = Σ_p param^pow[p] · M[p][j], with param vector [u³ u² u 1]; i.e. the
// four basis weights for the control points at parameter u.
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

// Samples an R×R grid of a patch by evaluating the blending polynomial at each
// (s,t) directly. Appends R*R vertices row-major (si outer, ti inner) to `out`.
static void tessellateBlending(const float M[4][4], const std::vector<core::Point>& cp,
                               int R, std::vector<core::Point>& out) {
    for (int si = 0; si < R; ++si) {
        float bs[4]; blend(M, (float)si / (float)(R - 1), bs);
        for (int ti = 0; ti < R; ++ti) {
            float bt[4]; blend(M, (float)ti / (float)(R - 1), bt);
            out.push_back(evalPatch(cp, bs, bt));
        }
    }
}

// Samples an R×R grid of a patch purely by accumulating forward differences (no
// per-point polynomial evaluation), using DD = E·(M·G·Mᵀ)·Eᵀ per coordinate.
// Appends R*R vertices row-major (si outer, ti inner) to `out`.
static void tessellateForwardDiff(const float M[4][4], const std::vector<core::Point>& cp,
                                  int R, const float E[4][4], const float Et[4][4],
                                  std::vector<core::Point>& out) {
    float DD[3][4][4];
    for (int axis = 0; axis < 3; ++axis) {
        float C[4][4], EC[4][4];
        coeffMatrix(M, cp, axis, C);
        matMul(E, C, EC);
        matMul(EC, Et, DD[axis]);
    }
    float W[3][4][4];
    std::memcpy(W, DD, sizeof(DD));
    for (int si = 0; si < R; ++si) {
        // Row 0 holds the t-curve's forward-diff state for the current s.
        float f[3], d1[3], d2[3], d3[3];
        for (int a = 0; a < 3; ++a) {
            f[a]  = W[a][0][0]; d1[a] = W[a][0][1];
            d2[a] = W[a][0][2]; d3[a] = W[a][0][3];
        }
        for (int ti = 0; ti < R; ++ti) {
            out.push_back(core::Point(f[0], f[1], f[2]));
            for (int a = 0; a < 3; ++a) { f[a] += d1[a]; d1[a] += d2[a]; d2[a] += d3[a]; }
        }
        // Advance s by one step: forward-difference the rows of W.
        for (int a = 0; a < 3; ++a)
            for (int c = 0; c < 4; ++c) {
                W[a][0][c] += W[a][1][c];
                W[a][1][c] += W[a][2][c];
                W[a][2][c] += W[a][3][c];
            }
    }
}

std::vector<std::vector<core::Point>>
surfaceGridToPatches(int rows, int cols,
                     const std::vector<core::Point>& grid, int method) {
    std::vector<std::vector<core::Point>> patches;
    if (rows < 4 || cols < 4 || (int)grid.size() < rows * cols) return patches;

    // B-Spline windows slide by 1 (overlapping); Bezier patches join edge-to-edge
    // sharing boundary control points, so the window slides by 3.
    const int step = (method == BSPLINE) ? 1 : 3;
    for (int pr = 0; pr + 3 < rows; pr += step) {
        for (int pc = 0; pc + 3 < cols; pc += step) {
            std::vector<core::Point> patch;
            patch.reserve(16);
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    patch.push_back(grid[(pr + i) * cols + (pc + j)]);
            patches.push_back(std::move(patch));
        }
    }
    return patches;
}

SurfaceFactory::SurfaceFactory(const std::string& name, int rows, int cols,
                               const std::vector<core::Point>& grid,
                               int method, int technique, int resolution, bool filled, ImU32 color)
    : name_(name), rows_(rows), cols_(cols), grid_(grid), method_(method),
      technique_(technique), resolution_(resolution < 2 ? 2 : resolution),
      filled_(filled), color_(color) {}

Object SurfaceFactory::build() {
    Object obj;
    obj.name = name_;
    obj.type = ObjectType::SURFACE;
    obj.material.color = color_;
    obj.material.filled = filled_;

    const float (*M)[4] = (method_ == BSPLINE) ? M_BSPLINE : M_BEZIER;
    const int R = resolution_;

    // Forward-difference basis matrices (only used by the SURF_FORWARD_DIFF path).
    float E[4][4], Et[4][4];
    forwardDiffE(1.0f / (float)(R - 1), E);
    transpose(E, Et);

    core::Mesh& mesh = *obj.mesh;
    const auto patches = surfaceGridToPatches(rows_, cols_, grid_, method_);

    for (const auto& cp : patches) {
        if (cp.size() < 16) continue;            // skip malformed patch
        const uint32_t base = (uint32_t)mesh.vertices.size();

        // Both techniques append the same R×R sample grid row-major
        // (index(si, ti) = base + si*R + ti); they differ only in how they
        // evaluate it.
        if (technique_ == SURF_BLENDING)
            tessellateBlending(M, cp, R, mesh.vertices);
        else
            tessellateForwardDiff(M, cp, R, E, Et, mesh.vertices);

        if (filled_) {
            // Two triangles per grid cell, wound consistently so the accumulated
            // vertex normals agree across the whole patch (the shading itself is
            // two-sided, and surfaces are never back-face culled).
            for (int si = 0; si + 1 < R; ++si)
                for (int ti = 0; ti + 1 < R; ++ti) {
                    const uint32_t v00 = base + si * R + ti;
                    const uint32_t v01 = v00 + 1;
                    const uint32_t v10 = base + (si + 1) * R + ti;
                    const uint32_t v11 = v10 + 1;
                    mesh.tri_indices.emplace_back(v00, v10, v11);
                    mesh.tri_indices.emplace_back(v00, v11, v01);
                }
        } else {
            // Grid lines: iso-s curves (vary t) and iso-t curves (vary s).
            for (int si = 0; si < R; ++si)
                for (int ti = 0; ti + 1 < R; ++ti)
                    mesh.line_indices.push_back({ base + si * R + ti, base + si * R + ti + 1 });
            for (int ti = 0; ti < R; ++ti)
                for (int si = 0; si + 1 < R; ++si)
                    mesh.line_indices.push_back({ base + si * R + ti, base + (si + 1) * R + ti });
        }
    }

    meta_.rows           = rows_;
    meta_.cols           = cols_;
    meta_.control_points = grid_;
    meta_.method         = method_;
    meta_.technique      = technique_;
    meta_.resolution     = resolution_;
    meta_.filled         = filled_;
    return obj;
}

std::unique_ptr<Metadata> SurfaceFactory::takeMetadata() {
    return std::make_unique<SurfaceMetadata>(std::move(meta_));
}

} // namespace core
