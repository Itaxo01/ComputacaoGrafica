#pragma once
#include "GeometryView.hpp"
#include "ObjectSlice.hpp"
#include "RenderedObject.hpp" // SortedTri
#include "Shading.hpp"        // shadePhongRaw, packColor
#include "Mat4.hpp"
#include "Point.hpp"
#include "HostDevice.hpp"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Shared per-element pipeline bodies. Every function here is CG_HD, so the SAME
// code runs in a CPU loop (RendererTransform.cpp / RendererClipping.cpp /
// RasterizationEngine.cpp drivers) and in a CUDA kernel (CudaGeometry.cu /
// CudaRaster.cu). Nothing here touches std::vector or any host-only type — all
// state is reached through a GBView (raw pointers) or POD params.
//
// Stages with variable output (clipping, sorted-tri build) call a templated `Sink`:
//   sink.emitPoint(pos, obj) / emitLine(a, b, obj) / emitTri(ClipVert a,b,c, obj)
//   sink.emit(SortedTri)
// The CPU sink appends via std::atomic counters into pre-sized GeometryBuffers; the
// device sink via atomicAdd into device arrays. Both end up calling the writeX()
// helpers below, so the actual memory writes are shared too.
// ─────────────────────────────────────────────────────────────────────────────

// POD params (device-safe).
struct ClipBox { core::Point lo, hi; };
struct ClipVert { core::Point pos, world, normal; };

// ── small math ──
CG_HD inline core::Point ncsToViewport(const core::Point& p, float cw, float ch, float scale) {
    float sx = ((p.x + 1.0f) * 0.5f) * cw;
    float sy = (1.0f - (p.y + 1.0f) * 0.5f) * ch;
    return core::Point(sx * scale, sy * scale, p.z); // z kept for depth
}
CG_HD inline float edgeFunction(const core::Point& a, const core::Point& b, float px, float py) {
    return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
}

// ═══════════════════════════ transform stages ════════════════════════════════
// world = transform * model (affine); pos = ncs * world. ncs is VRC (perspective) or
// the window-NCS matrix (ortho) — both affine, so chaining two divides is exact.
CG_HD inline void transformVertex(const GBView& g, const ObjectSlice* slices,
                                  const core::mat4& ncs, bool shaded, size_t i) {
    const core::mat4& tf = slices[g.vobj[i]].transform;
    core::Point tv = tf * g.getPos((uint32_t)i);
    if (shaded) g.setWorld((uint32_t)i, tv);
    g.setPos((uint32_t)i, ncs * tv);
}
// Area-weighted world-space face normal of triangle t (for the smooth-normal scatter).
CG_HD inline core::Point faceNormalWorld(const GBView& g, size_t t) {
    core::Point A = g.getWorld(g.triIdx[3*t]), B = g.getWorld(g.triIdx[3*t+1]), C = g.getWorld(g.triIdx[3*t+2]);
    return cross(B - A, C - A);
}
CG_HD inline void normalizeVertexNormal(const GBView& g, size_t i) {
    core::Point n = g.getNormal((uint32_t)i);
    float l = sqrtf(dot(n, n));
    if (l > 1e-12f) g.setNormal((uint32_t)i, n / l);
}
CG_HD inline void projectVertex(const GBView& g, const core::mat4& mat, size_t i) {
    g.setPos((uint32_t)i, mat * g.getPos((uint32_t)i)); // perspective divide inside mat*Point
}
CG_HD inline void viewportVertex(const GBView& g, float cw, float ch, float scale, size_t i) {
    g.setPos((uint32_t)i, ncsToViewport(g.getPos((uint32_t)i), cw, ch, scale));
}

// ═══════════════════════════ clip math ═══════════════════════════════════════
CG_HD inline bool clipSegmentNear(core::Point& a, core::Point& b, float nz) {
    float da = a.z - nz, db = b.z - nz;
    if (da < 0.0f && db < 0.0f) return false;
    if (da >= 0.0f && db >= 0.0f) return true;
    float t = da / (da - db);
    core::Point hit(a.x + t*(b.x-a.x), a.y + t*(b.y-a.y), a.z + t*(b.z-a.z));
    if (da < 0.0f) a = hit; else b = hit;
    return true;
}
CG_HD inline bool lbTest(float p, float q, float& u1, float& u2) {
    if (p == 0.0f) { if (q < 0.0f) return false; }
    else { float r = q/p; if (p < 0.0f) { if (r>u2) return false; if (r>u1) u1=r; }
                          else          { if (r<u1) return false; if (r<u2) u2=r; } }
    return true;
}
CG_HD inline bool clipSegmentLB(core::Point& a, core::Point& b, const ClipBox& box) {
    float u1=0.0f, u2=1.0f, dx=b.x-a.x, dy=b.y-a.y;
    if (!lbTest(-dx, a.x-box.lo.x, u1, u2)) return false;
    if (!lbTest( dx, box.hi.x-a.x, u1, u2)) return false;
    if (!lbTest(-dy, a.y-box.lo.y, u1, u2)) return false;
    if (!lbTest( dy, box.hi.y-a.y, u1, u2)) return false;
    float ox=a.x, oy=a.y;
    if (u1>0.0f) { a.x=ox+u1*dx; a.y=oy+u1*dy; }
    if (u2<1.0f) { b.x=ox+u2*dx; b.y=oy+u2*dy; }
    return true;
}
CG_HD inline int csOut(const core::Point& p, const ClipBox& box) {
    int o=0; if (p.x<box.lo.x) o|=1; else if (p.x>box.hi.x) o|=2;
             if (p.y<box.lo.y) o|=4; else if (p.y>box.hi.y) o|=8; return o;
}
CG_HD inline bool clipSegmentCS(core::Point& a, core::Point& b, const ClipBox& box) {
    int o1=csOut(a,box), o2=csOut(b,box);
    while (true) {
        if (!(o1|o2)) return true;
        if (o1 & o2)  return false;
        float x=0,y=0; int oc = o2>o1 ? o2 : o1;
        if      (oc&8) { x=a.x+(b.x-a.x)*(box.hi.y-a.y)/(b.y-a.y); y=box.hi.y; }
        else if (oc&4) { x=a.x+(b.x-a.x)*(box.lo.y-a.y)/(b.y-a.y); y=box.lo.y; }
        else if (oc&2) { y=a.y+(b.y-a.y)*(box.hi.x-a.x)/(b.x-a.x); x=box.hi.x; }
        else if (oc&1) { y=a.y+(b.y-a.y)*(box.lo.x-a.x)/(b.x-a.x); x=box.lo.x; }
        if (oc==o1) { a.x=x; a.y=y; o1=csOut(a,box); } else { b.x=x; b.y=y; o2=csOut(b,box); }
    }
}
CG_HD inline ClipVert lerpCV(const ClipVert& a, const ClipVert& b, float t) {
    return { a.pos+(b.pos-a.pos)*t, a.world+(b.world-a.world)*t, a.normal+(b.normal-a.normal)*t };
}
// Sutherland-Hodgman of a polygon (in-place, fixed capacity 8). A triangle vs the
// 4 box edges yields <= 7 vertices. Returns the new vertex count (0 if fully out).
CG_HD inline int shClipTriangle(ClipVert* poly, int n, const ClipBox& box) {
    ClipVert tmp[8];
    for (int edge=0; edge<4; ++edge) {
        for (int i=0;i<n;i++) tmp[i]=poly[i];
        int in=n; n=0;
        for (int i=0;i<in;i++) {
            const ClipVert& cur  = tmp[i];
            const ClipVert& prev = tmp[(i+in-1)%in];
            bool ci, pi; float bound;
            if      (edge==0) { ci=cur.pos.x>=box.lo.x; pi=prev.pos.x>=box.lo.x; bound=box.lo.x; }
            else if (edge==1) { ci=cur.pos.x<=box.hi.x; pi=prev.pos.x<=box.hi.x; bound=box.hi.x; }
            else if (edge==2) { ci=cur.pos.y>=box.lo.y; pi=prev.pos.y>=box.lo.y; bound=box.lo.y; }
            else              { ci=cur.pos.y<=box.hi.y; pi=prev.pos.y<=box.hi.y; bound=box.hi.y; }
            bool horiz = (edge<2);
            // crossing parameter on the relevant axis
            float t = 0.0f;
            if (ci != pi) {
                t = horiz ? (bound - prev.pos.x)/(cur.pos.x - prev.pos.x)
                          : (bound - prev.pos.y)/(cur.pos.y - prev.pos.y);
            }
            if (ci && !pi)      { if(n<8) poly[n++]=lerpCV(prev,cur,t); if(n<8) poly[n++]=cur; }
            else if (ci)        { if(n<8) poly[n++]=cur; }
            else if (pi)        { if(n<8) poly[n++]=lerpCV(prev,cur,t); }
        }
        if (n==0) return 0;
    }
    return n;
}
CG_HD inline bool isFilled(const ObjectSlice& s) {
    return (s.type==core::ObjectType::POLYGON || s.type==core::ObjectType::MESH) && s.filled;
}

// ── output writers (shared by the host and device sinks) ──
CG_HD inline void writePoint(const GBView& out, uint32_t vbase, uint32_t pbase,
                             const core::Point& p, int obj) {
    out.setPos(vbase, p); out.pointIdx[pbase]=vbase; out.pointObj[pbase]=obj;
}
CG_HD inline void writeLine(const GBView& out, uint32_t vbase, uint32_t lbase,
                            const core::Point& a, const core::Point& b, int obj) {
    out.setPos(vbase,a); out.setPos(vbase+1,b);
    out.lineIdx[2*lbase]=vbase; out.lineIdx[2*lbase+1]=vbase+1; out.lineObj[lbase]=obj;
}
CG_HD inline void writeTri(const GBView& out, uint32_t vbase, uint32_t tbase,
                           const ClipVert& A, const ClipVert& B, const ClipVert& C, int obj) {
    out.setPos(vbase,A.pos); out.setPos(vbase+1,B.pos); out.setPos(vbase+2,C.pos);
    if (out.shaded) {
        out.setWorld(vbase,A.world);   out.setWorld(vbase+1,B.world);   out.setWorld(vbase+2,C.world);
        out.setNormal(vbase,A.normal); out.setNormal(vbase+1,B.normal); out.setNormal(vbase+2,C.normal);
    }
    out.triIdx[3*tbase]=vbase; out.triIdx[3*tbase+1]=vbase+1; out.triIdx[3*tbase+2]=vbase+2;
    out.triObj[tbase]=obj;
}

// ── per-primitive clip (emit through a templated Sink) ──
template<class Sink> CG_HD void clipPointNear(const GBView& g, size_t i, float nz, Sink& sink) {
    core::Point p = g.getPos(g.pointIdx[i]);
    if (p.z >= nz) sink.emitPoint(p, g.pointObj[i]);
}
template<class Sink> CG_HD void clipPointBox(const GBView& g, size_t i, const ClipBox& box, Sink& sink) {
    core::Point p = g.getPos(g.pointIdx[i]);
    if (p.x>=box.lo.x && p.x<=box.hi.x && p.y>=box.lo.y && p.y<=box.hi.y) sink.emitPoint(p, g.pointObj[i]);
}
template<class Sink> CG_HD void clipLineNear(const GBView& g, size_t i, float nz, Sink& sink) {
    core::Point a=g.getPos(g.lineIdx[2*i]), b=g.getPos(g.lineIdx[2*i+1]);
    if (clipSegmentNear(a,b,nz)) sink.emitLine(a,b,g.lineObj[i]);
}
template<class Sink> CG_HD void clipLineBox(const GBView& g, const ObjectSlice* slices, size_t i,
                                            const ClipBox& box, int mode, Sink& sink) {
    core::Point a=g.getPos(g.lineIdx[2*i]), b=g.getPos(g.lineIdx[2*i+1]);
    bool surv = isFilled(slices[g.lineObj[i]]) ? clipSegmentLB(a,b,box)
              : (mode==1 ? clipSegmentCS(a,b,box) : clipSegmentLB(a,b,box));
    if (surv) sink.emitLine(a,b,g.lineObj[i]);
}
template<class Sink> CG_HD void clipTriNear(const GBView& g, size_t i, float nz, Sink& sink) {
    uint32_t ia=g.triIdx[3*i], ib=g.triIdx[3*i+1], ic=g.triIdx[3*i+2];
    core::Point A=g.getPos(ia), B=g.getPos(ib), C=g.getPos(ic);
    if (A.z>=nz && B.z>=nz && C.z>=nz) {
        core::Point z;
        ClipVert va{A, g.shaded?g.getWorld(ia):z, g.shaded?g.getNormal(ia):z};
        ClipVert vb{B, g.shaded?g.getWorld(ib):z, g.shaded?g.getNormal(ib):z};
        ClipVert vc{C, g.shaded?g.getWorld(ic):z, g.shaded?g.getNormal(ic):z};
        sink.emitTri(va,vb,vc,g.triObj[i]);
    }
}
template<class Sink> CG_HD void clipTriBox(const GBView& g, const ObjectSlice* slices, size_t i,
                                           const ClipBox& box, Sink& sink) {
    if (!isFilled(slices[g.triObj[i]])) return;
    uint32_t ia=g.triIdx[3*i], ib=g.triIdx[3*i+1], ic=g.triIdx[3*i+2];
    core::Point z;
    ClipVert poly[8];
    poly[0]={g.getPos(ia), g.shaded?g.getWorld(ia):z, g.shaded?g.getNormal(ia):z};
    poly[1]={g.getPos(ib), g.shaded?g.getWorld(ib):z, g.shaded?g.getNormal(ib):z};
    poly[2]={g.getPos(ic), g.shaded?g.getWorld(ic):z, g.shaded?g.getNormal(ic):z};
    int n = shClipTriangle(poly, 3, box);
    if (n < 3) return;
    for (int k=1; k+1<n; ++k) sink.emitTri(poly[0], poly[k], poly[k+1], g.triObj[i]); // convex fan
}

// ── build one screen-space SortedTri (NCS -> viewport, cull) ──
template<class Sink> CG_HD void buildSortedTriangle(const GBView& g, const ObjectSlice* slices, size_t t,
                                                    float cw, float ch, float scale,
                                                    int meshType, bool cull, bool cullCcw, Sink& sink) {
    const ObjectSlice& s = slices[g.triObj[t]];
    bool cullThis = cull && ((int)s.type == meshType);
    uint32_t ia=g.triIdx[3*t], ib=g.triIdx[3*t+1], ic=g.triIdx[3*t+2];
    core::Point A=g.getPos(ia), B=g.getPos(ib), C=g.getPos(ic); // NCS: z is depth
    core::Point va=ncsToViewport(A,cw,ch,scale), vb=ncsToViewport(B,cw,ch,scale), vc=ncsToViewport(C,cw,ch,scale);
    float area = (vb.x-va.x)*(vc.y-va.y) - (vb.y-va.y)*(vc.x-va.x);
    if (cullThis) { bool back = cullCcw ? (area<=0.0f) : (area>=0.0f); if (back) return; }
    SortedTri tt;
    tt.a=va; tt.b=vb; tt.c=vc; tt.color=s.color;
    tt.depth=(A.z+B.z+C.z)/3.0f; tt.za=A.z; tt.zb=B.z; tt.zc=C.z;
    if (g.shaded) {
        tt.P[0]=g.getWorld(ia); tt.P[1]=g.getWorld(ib); tt.P[2]=g.getWorld(ic);
        tt.N[0]=g.getNormal(ia); tt.N[1]=g.getNormal(ib); tt.N[2]=g.getNormal(ic);
        tt.mat=s.shadeMat;
    }
    sink.emit(tt);
}

// ═══════════════════════════ triangle rasterization ══════════════════════════
// Per-triangle fill into rows [y_lo, y_hi). For each covered pixel computes the color
// (mode 0 flat-color / 1 flat-shaded / 2 gouraud / 3 phong) and depth, then calls
// commit(x, y, z, color). The commit decides how to store it: the CPU writes to the
// Framebuffer (depth-tested or not); the GPU does a packed atomicMin z-buffer.
template<class Commit>
CG_HD void rasterizeTriangleInto(const SortedTri& tr, int W, int y_lo, int y_hi, int mode,
                                 const core::Point& eye, const core::Color3& ambient,
                                 const core::Light* lights, int nLights, Commit commit) {
    float minx=fminf(tr.a.x,fminf(tr.b.x,tr.c.x)), maxx=fmaxf(tr.a.x,fmaxf(tr.b.x,tr.c.x));
    float miny=fminf(tr.a.y,fminf(tr.b.y,tr.c.y)), maxy=fmaxf(tr.a.y,fmaxf(tr.b.y,tr.c.y));
    int x0=(int)floorf(minx); if (x0<0) x0=0;
    int x1=(int)ceilf(maxx);  if (x1>W-1) x1=W-1;
    int yy0=(int)floorf(miny); if (yy0<y_lo) yy0=y_lo;
    int yy1=(int)ceilf(maxy);  if (yy1>y_hi-1) yy1=y_hi-1;
    if (x0>x1 || yy0>yy1) return;
    float area = edgeFunction(tr.a, tr.b, tr.c.x, tr.c.y);
    if (area == 0.0f) return;
    float invArea = 1.0f/area;

    ImU32 flatCol = tr.color;
    core::Color3 gc0, gc1, gc2;
    if (mode==1) {
        core::Point fn  = cross(tr.P[1]-tr.P[0], tr.P[2]-tr.P[0]);
        core::Point cen = (tr.P[0]+tr.P[1]+tr.P[2]) * (1.0f/3.0f);
        flatCol = packColor(shadePhongRaw(cen, fn, tr.mat, eye, ambient, lights, nLights));
    } else if (mode==2) {
        gc0 = shadePhongRaw(tr.P[0], tr.N[0], tr.mat, eye, ambient, lights, nLights);
        gc1 = shadePhongRaw(tr.P[1], tr.N[1], tr.mat, eye, ambient, lights, nLights);
        gc2 = shadePhongRaw(tr.P[2], tr.N[2], tr.mat, eye, ambient, lights, nLights);
    }
    for (int y=yy0; y<=yy1; ++y) {
        float py=(float)y+0.5f;
        for (int x=x0; x<=x1; ++x) {
            float px=(float)x+0.5f;
            float w0=edgeFunction(tr.b,tr.c,px,py);
            float w1=edgeFunction(tr.c,tr.a,px,py);
            float w2=edgeFunction(tr.a,tr.b,px,py);
            bool inside=(w0>=0&&w1>=0&&w2>=0)||(w0<=0&&w1<=0&&w2<=0);
            if (!inside) continue;
            ImU32 col;
            if (mode==0)      col=tr.color;
            else if (mode==1) col=flatCol;
            else {
                float l0=w0*invArea, l1=w1*invArea, l2=w2*invArea;
                if (mode==2) col=packColor({ l0*gc0.r+l1*gc1.r+l2*gc2.r,
                                             l0*gc0.g+l1*gc1.g+l2*gc2.g,
                                             l0*gc0.b+l1*gc1.b+l2*gc2.b });
                else {
                    core::Point Pp=tr.P[0]*l0+tr.P[1]*l1+tr.P[2]*l2;
                    core::Point Np=tr.N[0]*l0+tr.N[1]*l1+tr.N[2]*l2;
                    col=packColor(shadePhongRaw(Pp, Np, tr.mat, eye, ambient, lights, nLights));
                }
            }
            float z=(w0*tr.za+w1*tr.zb+w2*tr.zc)*invArea;
            commit(x, y, z, col);
        }
    }
}
