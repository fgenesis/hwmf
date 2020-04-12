// Low-level rasterizer. For the high-level crap, see drawmesh.h

#pragma once

#include "cfg-demo.h"
#include "../demomath/fgmath.h"
#include "vertex.h"

#include <limits.h>

namespace raster {

typedef LCD::ColorType Color;

struct Tri
{
    uint8_t a, b, c;
};

struct Params
{
    int8_t align; // must be power of 2
    int8_t alignoffs;
    int8_t incr;
    //uint8_t face; // 0: only backface, 1: only frontface, else: anything
    //uint8_t backfaceoffs;
    //uint8_t backfaceColorOffs;

    // FIXME: remove those from here
    uint8_t rubmul;
    uint8_t glitchmul;
};

struct AABB
{
    FORCEINLINE AABB() : start(SHRT_MAX), end(SHRT_MIN) {}
    FORCEINLINE AABB(const ivec2& v) : start(v), end(v) {}
    ivec2 start, end;
    void extend(const ivec2& p)
    {
        extendMin(p);
        extendMax(p);
    }
    void extend(const AABB& p)
    {
        extendMin(p.start);
        extendMax(p.end);
    }
    void widen(uint8_t w)
    {
        start.x -= w;
        start.y -= w;
        end.x += w;
        end.y += w;
    }
    void clampToLCD()
    {
        //shrink(ivec2(0,0), ivec2(LCD::XMAX, LCD::YMAX));

        start.x = vclamp<int16_t>(start.x, 0, LCD::XMAX);
        start.y = vclamp<int16_t>(start.y, 0, LCD::YMAX);
        end.x   = vclamp<int16_t>(end.x, 0, LCD::XMAX);
        end.y   = vclamp<int16_t>(end.y, 0, LCD::YMAX);
    }
    FORCEINLINE void shrink(const ivec2& b, const ivec2& e)
    {
        start.x = vmax(start.x, b.x);
        start.y = vmax(start.y, b.y);
        end.x = vmin(end.x, e.x);
        end.y = vmin(end.y, e.y);
    }
    FORCEINLINE void extendMin(const ivec2& p)
    {
        start.x = vmin(start.x, p.x);
        start.y = vmin(start.y, p.y);
    }
    FORCEINLINE void extendMax(const ivec2& p)
    {
        end.x = vmax(end.x, p.x);
        end.y = vmax(end.y, p.y);
    }
    FORCEINLINE ivec2 size() const 
    {
        return end - start + 1;
    }
    FORCEINLINE bool valid() const
    {
        return start.x <= end.x;
    }
};

struct ClearHelperBase
{
    void _clearH(Color col, uint8_t k, uint8_t N) const;
    void _clearV(Color col, uint8_t k, uint8_t N) const;
    void _recalc(const AABB *aa, uint8_t N, uint8_t padding);
    AABB bound;
};

template<uint8_t N> // must be power of 2
struct ClearHelperN : ClearHelperBase
{
    AABB aa[N];
    uint8_t r, w;

    inline void add(const AABB& a, uint8_t padding)
    {
        aa[w++ & (N - 1)] = a;
        this->_recalc(&aa[0], N, padding);
    }
};

template<uint8_t N, uint8_t stride = 1, uint8_t offset = 0> // N must be power of 2
struct ClearHelper : ClearHelperN<N>
{
    inline void clear(Color col)
    {
        uint8_t k = (this->r + 1) & (N - 1);
        this->r = k;
        this->_clearV(col, k*stride+offset, N);
    }
};

template<uint8_t N, uint8_t stride = 1, uint8_t offset = 0> // N must be power of 2
struct ClearHelperH : ClearHelperN<N>
{
    inline void clear(Color col)
    {
        uint8_t k = (this->r + 1) & (N - 1);
        this->r = k;
        this->_clearH(col, k*stride+offset, N);
    }
};

struct CastY
{
    void init(const ivec2 v, const ivec2 m);
    bool cast(ivec2& vIn, const int y);
private:
    uint8_t looptype;
    uint16_t _dx2, _dy2;
    int16_t _err;
    int8_t _sx;
    ivec2 _m;
};

static FORCEINLINE uint8_t isFrontFace(const ivec2 v0, const ivec2 v1, const ivec2 v2)
{
    return (v1.y - v0.y) * (v2.x - v1.x) - (v1.x - v0.x) * (v2.y - v1.y) < 0;
}

static FORCEINLINE void sortverts(ivec2& top, ivec2& midL, ivec2& btmR)
{
    if(top.y > btmR.y)
        vswap(top, btmR);
    if(top.y > midL.y)
        vswap(top, midL);
    if(midL.y > btmR.y)
        vswap(midL, btmR);
}

AABB getVertexListAABB(const ivec2 *pv, const uint16_t n);


// low-level draw function.
// vertices must be passed properly sorted. y >= 0.
template<typename Render>
static FORCEINLINE void drawTriLow(Render& render, const ivec2 top, const ivec2 midL, const ivec2 btmR, int y, const uint8_t incr, const uint8_t colid)
{
    FGLCD_ASSERT(top.y <= midL.y && midL.y <= btmR.y, "drawtrio");

    const int h = vmin<int>(btmR.y, render.height());

    if(y >= h)
        return;

    // all points in order now
    ivec2 pL = top, pR = top;
    CastY c1, c2;
    c1.init(pL, midL);
    c2.init(pR, btmR);
    const typename Render::ColorType col = render.getColor(colid);
    uint8_t once = 0;
    do
    {
        if(c1.cast(pL, y))
        {
            if(once)
                return;
            once = 1;
            pL = midL; // this avoids disconnected edges
            c1.init(pL, btmR);
        }
        c2.cast(pR, y);

        if(pL.x != pR.x)
        {
            int x1 = pL.x;
            int x2 = pR.x;
            makeminmax(x1, x2);
            render.drawScanline(x1, x2, y, col);
        }

        y += incr;
    }
    while(y < h);
}

// the larger this number, the more to dampen the color
uint8_t getScreeenspaceShade(vtx::Zval za, vtx::Zval zb, vtx::Zval zc);

struct TriFilterResult
{
    uint16_t nFront;
    uint16_t nBack;
};

// Filters indices of visible tris into triidxs:
// - At the front of the list when it's a front-facing triangle
// - At the back of the list when it's a back-facing triangle
TriFilterResult filterTris(uint8_t *triidxs, const ivec2 * const vs, const uint8_t *clipbuf, const Tri *tris, uint16_t nTris);

// Pass a filtered list of indices to sort.
// After sorting the back-most triangles will be first in the list.
// (Use this after filtering to save sorting time)
void sortTrisZ(uint8_t *triidxs, uint8_t n, const Tri *tris, uint16_t maxtris, const vtx::Zval * const zv);

template<typename LESS>
void sortTris(uint8_t *triidxs, uint8_t n, const Tri *tris, const LESS& cmp)
{
    struct Helper
    {
        uint8_t * const triidxs;
        const Tri * const tris;
        const LESS& cmp;

        FORCEINLINE Helper(uint8_t *triidxs, const Tri *tris, const LESS& cmp)
            : triidxs(triidxs), tris(tris), cmp(cmp)
        {}

        FORCEINLINE bool operator() (uint8_t a, uint8_t b) const
        {
            return cmp(tris[triidxs[a]], tris[triidxs[b]]);
        }
    };

    Helper h(triidxs, tris, cmp);
    fgstd::sort(triidxs, triidxs + n, h);
}

void drawWireframe(const uint8_t *triidxs, uint16_t n, Color col, const raster::Tri *tris, const ivec2 *vs);

// pass tris=NULL for non-indexed rendering
// pass zbuf=NULL for unshaded rendering
template<typename Render>
void drawTriangles(Render& render, const uint8_t *triidxs, uint16_t n, const ivec2 *vs, const Tri *tris, const uint8_t * const colorIdxs, const vtx::Zval *zbuf, const Params& rp, uint8_t yoffs, uint8_t shadelevels, uint8_t upshade, uint8_t downshade, uint8_t coloffs)
{
    if(!zbuf)
        shadelevels = 0;

    for(uint16_t i = 0; i < n; ++i)
    {
        uint8_t triidx = triidxs[i];
        ivec2 v0(noinit), v1(noinit), v2(noinit);
        if(tris) // indexed rendering // this is a constant jump but it's here to save space (compared to specialization). on AVR branches are cheap so whatev.
        {
            const Tri& tri = tris[triidx];
            v0 = vs[tri.a];
            v1 = vs[tri.b];
            v2 = vs[tri.c];
        }
        else // triangle soup without index
        {
            v0 = *vs++;
            v1 = *vs++;
            v2 = *vs++;
        }

        sortverts(v0, v1, v2);

        // Convert mesh color index to palette index
        uint8_t colid;
        if(shadelevels)
        {
            const uint8_t firstcolor = shadelevels + coloffs;
            const uint8_t colordist = shadelevels << 1;
            const uint8_t palbase = firstcolor + colordist * colorIdxs[triidx];

            vtx::Zval za, zb, zc;
            if(tris)
            {
                const Tri& tri = tris[triidx];
                za = zbuf[tri.a];
                zb = zbuf[tri.b];
                zc = zbuf[tri.c];
            }
            else
            {
                za = *zbuf++;
                zb = *zbuf++;
                zc = *zbuf++;
            }
            uint8_t shade = raster::getScreeenspaceShade(za, zb, zc);

            colid = palbase + scale8(shadelevels, upshade) - scale8(shade, downshade);
        }
        else
            colid = colorIdxs[triidx];

        int y = ((vmax<int>(v0.y, 0)-1) | (rp.align - 1)) + ((rp.incr) + rp.alignoffs);
        y += yoffs;

        drawTriLow(render, v0, v1, v2, y, rp.incr, colid);
    }
}

} // end namespace raster
