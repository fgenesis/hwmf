#include "raster.h"
#include "draw.h"

raster::TriFilterResult raster::filterTris(uint8_t *triidxs, const ivec2 * const vs, const uint8_t *clipbuf, const raster::Tri *tris, uint16_t nTris)
{
    uint8_t nFront = 0, nBack = 0;
    for(uint16_t i = 0; i < nTris; ++i)
    {
        const raster::Tri tri = tris[i];

        // Early clipping check
        if(clipbuf)
        {
            uint8_t clip0 = clipbuf[tri.a];
            uint8_t clip1 = clipbuf[tri.b];
            uint8_t clip2 = clipbuf[tri.c];

            if(!(clip0 | clip1 | clip2))
                goto accept; // All verts inside of viewport

            if(clip0 & clip1 & clip2)
                continue; // All verts are outside of the viewport on the same side

            if(clip0 | clip1 | clip2)
                continue;
        }

accept:

        ivec2 v0 = vs[tri.a];
        ivec2 v1 = vs[tri.b];
        ivec2 v2 = vs[tri.c];

        if(raster::isFrontFace(v0, v1, v2))
            triidxs[nFront++] = i;
        else
            triidxs[nTris - ++nBack] = i;
    }

    raster::TriFilterResult res;
    res.nFront = nFront;
    res.nBack = nBack;
    return res;
}

static FORCEINLINE vtx::Zval calcTriDepth(uint16_t triidx, const raster::Tri *tris, const vtx::Zval *zs)
{
    const raster::Tri t = tris[triidx];
    return zs[t.a] + zs[t.b] + zs[t.c]; // proper average isn't necessary
}

struct TriCmp
{
    const vtx::Zval * const _tz;
    const raster::Tri * const _tri;
    TriCmp(const vtx::Zval *tz, const raster::Tri *tri) : _tz(tz), _tri(tri) {}
    bool operator()(uint8_t A, uint8_t B) const
    {
        return calcTriDepth(A, _tri, _tz) < calcTriDepth(B, _tri, _tz);
    }
};

struct TriCmpLUT
{
    const vtx::Zval * const _lut;
    TriCmpLUT(const vtx::Zval *lut) : _lut(lut) {}
    bool operator()(uint8_t A, uint8_t B) const
    {
        return _lut[A] < _lut[B];
    }
};

void raster::sortTrisZ(uint8_t *triidxs, uint8_t n, const raster::Tri *tris, uint16_t maxtris, const vtx::Zval *zv)
{
    FGLCD_ASSERT(zv, "sortidx");

#if CFG_FAST_TRIANGLE_SORTING+0
    vtx::Zval *lut = (vtx::Zval*)StackAlloc(maxtris * sizeof(vtx::Zval));
    for(uint8_t i = 0; i < n; ++i)
    {
        uint8_t triidx = triidxs[i];
        lut[triidx] = calcTriDepth(triidx, tris, zv);
    }
    TriCmpLUT cmp(lut);
#else
    TriCmp cmp(zv, tris);
#endif

    fgstd::sort(triidxs, triidxs + n, cmp);
}

uint8_t raster::getScreeenspaceShade(vtx::Zval za, vtx::Zval zb, vtx::Zval zc)
{
    sort3(za, zb, zc);
    return uint8_t(zc - za); // known to be positive
}

void raster::drawWireframe(const uint8_t *triidxs, uint16_t n, Color col, const raster::Tri *tris, const ivec2 *vs)
{
    if(tris)
    {
        for(uint16_t i = 0; i < n; ++i)
        {
            const raster::Tri tri = tris[triidxs[i]];
            const ivec2 v0 = vs[tri.a];
            const ivec2 v1 = vs[tri.b];
            const ivec2 v2 = vs[tri.c];

            // Doesn't 100% prevent drawing a line twice but makes a big difference
            if(tri.a < tri.b)
                Draw::drawline(v0.x, v0.y, v1.x, v1.y, col);
            if(tri.b < tri.c)
                Draw::drawline(v1.x, v1.y, v2.x, v2.y, col);
            if(tri.c < tri.a)
                Draw::drawline(v2.x, v2.y, v0.x, v0.y, col);
        }
    }
    else
    {
        for(uint16_t i = 0; i < n; ++i)
        {
            uint16_t triidx = triidxs[i] * 3;
            const ivec2 v0 = vs[triidx];
            const ivec2 v1 = vs[triidx+1];
            const ivec2 v2 = vs[triidx+2];
            Draw::drawline(v0.x, v0.y, v1.x, v1.y, col);
            Draw::drawline(v1.x, v1.y, v2.x, v2.y, col);
            Draw::drawline(v2.x, v2.y, v0.x, v0.y, col);
        }
    }
}


void raster::CastY::init(const ivec2 v, const ivec2 m)
{
    //FGLCD_ASSERT(v.y <= m.y, "rciy");
    uint16_t dx = vabs(m.x - v.x);
    uint16_t dy = m.y - v.y; // _sy == 1
    _sx = v.x < m.x ? 1 : -1;
    _m = m;
    looptype = dy > dx;
    if(looptype)
        vswap(dy, dx);

    _dx2 = dx * 2;
    _dy2 = dy * 2;
    _err = _dy2 - dx;
    if(!_dx2)
        looptype = 2;
}

// true if goal reached, false if aborted due to y
bool raster::CastY::cast(ivec2& vIn, const int y)
{
    const int yy = vmin<int>(y, _m.y);
    ivec2 v = vIn;
    switch(looptype)
    {
        case 0:
            while(v.y != yy)
            {
                while(_err >= 0)
                {
                    ++v.y;
                    _err -= _dx2;
                }
                v.x += _sx;
                _err += _dy2;
            }
            break;

        case 1:
            while(v.y != yy)
            {
                while(_err >= 0)
                {
                    v.x += _sx;
                    _err -= _dx2;
                }
                ++v.y;
                _err += _dy2;
            }
            break;

        case 2:
            vIn.x = _m.x;
            //FGLCD_ASSERT(v.y == _m.y, "yywat");
            return true;
    }

    //FGLCD_ASSERT(v.y == y || v.y == _m.y, "ywat");
    vIn = v;
    return (v.y == _m.y);
}

raster::AABB raster::getVertexListAABB(const ivec2 *pv, const uint16_t n)
{
    //ASSERT(n);
    raster::AABB ret(pv[0]); // HACK: don't n == 0;
    for(uint16_t i = 1; i < n; ++i)
        ret.extend(pv[i]);
    return ret;
}

/*
static AABB getTrisAABB(const ivec2 *pv, const uint8_t *pidx, const uint8_t n, LCD::ColorType col)
{
    AABB ret;
    for(uint8_t i = 0; i < n; ++i, pidx += 3) // FIXME: autoinc
    {
        const ivec2 v0 = pv[pidx[0]], v1 = pv[pidx[1]], v2 = pv[pidx[2]];
        const ivec2 pmin(vmin3(v0.x, v1.x, v2.x), vmin3(v0.y, v1.y, v2.y));
        const ivec2 pmax(vmax3(v0.x, v1.x, v2.x), vmax3(v0.y, v1.y, v2.y));
        ret.extendMin(pmin);
        ret.extendMax(pmax);
    }
    ret.shrink(ivec2(0,0), ivec2(LCD::XMAX, LCD::YMAX));
    return ret;
}
*/

void raster::ClearHelperBase::_clearV(Color col, uint8_t k, uint8_t N) const
{
    const raster::AABB bb = bound;
    uint16_t x = (bb.start.x & ~(N - 1)) + k;
    const uint16_t end = uint16_t(bb.end.x);
    for( ; x <= end; x += N)
        LCD::fillrectfromto(x, bb.start.y, x, bb.end.y, col);
}

void raster::ClearHelperBase::_clearH(Color col, uint8_t k, uint8_t N) const
{
    const raster::AABB bb = bound;
    uint16_t y = (bb.start.y & ~(N - 1)) + k;
    const uint16_t end = uint16_t(bb.end.y);
    for( ; y <= end; y += N)
        LCD::fillrectfromto(bb.start.x, y, bb.end.x, y, col);
}

void raster::ClearHelperBase::_recalc(const AABB * aa, uint8_t N, uint8_t padding)
{
    AABB bb;
    for(uint8_t i = 1; i < N; ++i)
        bb.extend(aa[i]);
    if(padding)
        bb.widen(padding);
    bb.clampToLCD();
    bound = bb;
}
