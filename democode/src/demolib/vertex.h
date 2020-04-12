#pragma once

#include "../demomath/mat4.h"
#include "sort.h"
#include <stdlib.h>
#include "../demomath/fgmath.h"


namespace vtx {

#if CFG_FLOAT_DEPTH+0
typedef float Zval;
static FORCEINLINE Zval calcDepth(const vec4& v) { return Zval(v.z.tofloat() / v.w.tofloat()) }
#else
typedef int16_t Zval;
static FORCEINLINE Zval calcDepth(const vec4& v) { return v.z.int16div(v.w); }
#endif

enum Constants
{
    COORD_BITS = 8 * sizeof(ivec2::value_type),
    SCREENSPACE_BITS = COORD_BITS - 2, // this also needs to be chosen so that reasonable values of v.w don't overflow the multiplication
    SCREENSPACE_MASK = (1 << SCREENSPACE_BITS) - 1,
    MAXLCDSIZE = LCD::WIDTH > LCD::HEIGHT ? LCD::WIDTH : LCD::HEIGHT,
    GUARDBAND_MULT = SCREENSPACE_MASK / MAXLCDSIZE,
    GUARDBAND_SIZE = MAXLCDSIZE * GUARDBAND_MULT,

    // Cohen-Sutherland outcodes
    OUT_U = 1,
    OUT_D = 2,
    OUT_L = 4,
    OUT_R = 8,
};

static FORCEINLINE ivec2 project1(const vec4& v)
{
    return ivec2(v.x.int16div(v.w), v.y.int16div(v.w));
}


static inline uint8_t calcOutcode(const vec4& v)
{
    uint8_t ret = 0;
    if(v.w > 0) // inside frustum?
    {
        // round up int part; it's known to be positive and this avoids the case R=L=0 caused by v.w < 1.0
        const int32_t R = (v.w.intpart() + 1) * int32_t(GUARDBAND_SIZE);
        const int32_t L = -R;
        const int16_t xi = v.x.intpart();
        const int16_t yi = v.y.intpart();
        if(xi < L)
            ret |= OUT_L;
        if(yi < L)
            ret |= OUT_U;
        if(R < xi)
            ret |= OUT_R;
        if(R < yi)
            ret |= OUT_D;
    }
    else
        --ret;
    
    return ret;
}

template<typename M>
static FORCEINLINE vec4 transform1(const vec3& v3, const M& m)
{
    vec4 v = m * vec4(v3.x, v3.y, v3.z, fp1616(1));
    if(v.w.f.i == 0)
        v.w.f.i = 1;
    return v;
}

// vec3 variant (auto-transform to vec4(v3, 1.0f) for homogenous coords)
template<typename M>
static void transform_and_project(ivec2 * dst, uint8_t *clipstuff, Zval *zbuf, const svec3 *src, const M& m, const unsigned n, const ivec2& viewport)
{
    for(unsigned i = 0; i < n; ++i)
    {
        //assert((m.template get<3, 3>()) != M::Zero());
        //assert(v.w != M::Zero());

        const svec3 v3 = src[i];
        vec4 vv(v3.x, v3.y, v3.z, fp1616(1));
        vec4 v = m * vv;

        // clipping
        if(clipstuff)
            clipstuff[i] = calcOutcode(v);
        else if(v.w.f.i == 0) // will clip when w == 0, but otherwise have to check
            v.w.f.i = 1;

        if(zbuf)
            zbuf[i] = calcDepth(v);

        dst[i] = project1(v) + viewport;
    }
}

};
