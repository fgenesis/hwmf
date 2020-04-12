#define NO_SDL

#include "fgmath.h"
#include "lookup.h"

// via https://github.com/chmike/fpsqrt/blob/master/fpsqrt.c
fp1616 sqrt(fp1616 x)
{
    uint32_t r = x.raw();
    uint32_t b = 0x40000000;
    uint32_t q = 0;
    while( b > 0x40 )
    {
        uint32_t t = q + b;
        if( r >= t )
        {
            r -= t;
            q = t + b; // equivalent to q += 2*b
        }
        r <<= 1;
        b >>= 1;
    }
    q >>= 8;
    return fp1616::raw(q);
}

uint8_t sqrt16(uint16_t A)
{
    uint8_t bit = 0x80;
    uint8_t x = 0;
    while(bit)
    {
        x += bit;
        if(A < uint16_t(x) * x)
            x -= bit;
        bit >>= 1;
    }
    return x;
}

// might check https://stackoverflow.com/questions/6286450/inverse-sqrt-for-fixed-point eventually,
// but that didn't work
fp1616 invsqrt(fp1616 input)
{
    return input.f.i ? fp1616(1) / sqrt(input) : 0;
}

static const constexpr float NEAR = 0.01f;
static const constexpr float FAR = 100;

static const constexpr fp1616 near = fp1616(NEAR);
static const constexpr fp1616 invnear = fp1616(1.0f / NEAR);
static const constexpr fp1616 one = fp1616(1);
static const constexpr fp1616 minusone = fp1616(-1);
static const constexpr fp1616 two = fp1616(2);
static const constexpr fp1616 half = fp1616(0.5f);
static const constexpr fp1616 twonear = fp1616(NEAR * 2);
static const constexpr fp1616 eps = fp1616::raw(1);
static const constexpr fp1616 epsMinusOne = eps - 1;
static const constexpr fp1616 epsMinusTwo = eps - 2;
static const constexpr fp1616 epsMinusTwoTimesNear = fp1616::cmul(epsMinusTwo, near); //fp1616::raw(0xffffe668);     // (eps - 2) * near

// sucks. fucked up.
/*
tmat4_persp<fp1616> perspective(uint8_t fovy, fp1616 aspect)
{
    const float tanHalfFovy = tan(fovy); //tan88slow(fovy).tofloat();
    const fp1616 scale = aspect / tanHalfFovy;

    tmat4_persp<fp1616> r;
    //r.template set<0, 0>(1.0f / (aspect.tofloat() * tanHalfFovy));
    //r.template set<1, 1>(1.0f / (tanHalfFovy));
    r.template set<0, 0>(scale);
    r.template set<1, 1>(scale);
    //r.template set<3, 2>(-1);  // encoded in type
    r.template set<2, 2>(- FAR / (NEAR - FAR));
    r.template set<2, 3>(-(FAR * NEAR) / (FAR - NEAR));
    //r.template set<2, 2>(0); // can just 0 since we don't need the depth, apparently... so no more near/far as well
    //r.template set<3, 2>(0);
    return r;
}
*/
#if 1
// NO-DIVISION TEMP VERSION
tmat4_persp<fp1616> tweakedInfinitePerspective(uint8_t fovy, fp1616 aspect)
{
    const fp1616 itanv = fp1616(invtan88slow(fovy));
    const fp1616 irange = invnear * itanv;
    const fp1616 tmp = near * irange;

    tmat4_persp<fp1616> r;
    r.template set<1, 1>(tmp);
    r.template set<0, 0>(tmp * (one / aspect)); // TODO: should be done by caller
    //r.template set<2, 2>(epsMinusOne); // close enough to -1, also encoded in type
    //r.template set<3, 2>(minusone); // encoded in type
    r.template set<2, 3>(epsMinusTwoTimesNear);
    return r;
}

#else
// SLOW, GOOD VERSION
tmat4_persp<fp1616> tweakedInfinitePerspective(uint8_t fovy, fp1616 aspect)
{
    const fp1616 tanv = fp1616(tan88slow(fovy));
    const fp1616 range = near * tanv;
    const fp1616 tmp =  range * aspect;
    const fp1616 span = tmp + tmp;

    tmat4_persp<fp1616> r;
    r.template set<0, 0>(twonear / span);
    r.template set<1, 1>(twonear / (range + range));
    //r.template set<2, 2>(epsMinusOne); // close enough to -1, also encoded in type
    //r.template set<3, 2>(minusone); // encoded in type
    r.template set<2, 3>(epsMinusTwoTimesNear);
    return r;
}
#endif

tmat4_persp<fp1616> tweakedInfinitePerspective_InvAspect(uint8_t fovy, fp1616 iaspect)
{
    const fp1616 itanv = fp1616(invtan88slow(fovy));
    const fp1616 irange = invnear * itanv;
    const fp1616 tmp = near * irange;

    tmat4_persp<fp1616> r;
    r.template set<1, 1>(tmp);
    r.template set<0, 0>(tmp * iaspect); // TODO: should be done by caller
    //r.template set<2, 2>(epsMinusOne); // close enough to -1, also encoded in type
    //r.template set<3, 2>(minusone); // encoded in type
    r.template set<2, 3>(epsMinusTwoTimesNear);
    return r;
}

tmat4_persp<fp1616> tweakedInfinitePerspective_NoAspect(uint8_t fovy)
{
    const fp1616 itanv = fp1616(invtan88slow(fovy));

    tmat4_persp<fp1616> r;
    r.template set<1, 1>(itanv);
    r.template set<0, 0>(itanv);
    //r.template set<2, 2>(epsMinusOne); // close enough to -1, also encoded in type
    //r.template set<3, 2>(minusone); // encoded in type
    r.template set<2, 3>(epsMinusTwoTimesNear);
    return r;
}

// NOT TESTED YET
/*
tmat4_infperspLH<fp1616> infinitePerspectiveLH(uint8_t fovy, fp1616 aspect)
{
    static constexpr fp1616 near = fp1616(NEAR);
    static constexpr fp1616 twonear = fp1616(NEAR * 2);
    static constexpr fp1616 minustwonear = fp1616(NEAR * -2);

    const fp88 tanv = tan88slow(fovy);
    const fp1616 range = near * fp1616(tanv);
    const fp1616 tmp =  range * aspect;
    const fp1616 span = tmp + tmp;

    tmat4_infperspLH<fp1616> r;
    r.template set<0, 0>(twonear / span);
    r.template set<1, 1>(twonear / (range + range));
    r.template set<3, 2>(1); // this fixes everything backwards
    r.template set<2, 3>(minustwonear);
    return r;
}
*/
