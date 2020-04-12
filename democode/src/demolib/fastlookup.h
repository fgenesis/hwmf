#pragma once

#include "../demomath/lookup.h"
#include "scratch.h"

template<typename T>
static FORCEINLINE T *pageselect(T *p, uint8_t page)
{
    union
    {
        T *p;
        char c[sizeof(void*)];
    } u;
    u.p = p;
    u.c[1] += page;
    return u.p;
}

template<typename T>
static FORCEINLINE T *pageindex(T *p, uint8_t i)
{
    union
    {
        T *p;
        char c[sizeof(void*)];
    } u;
    u.p = p;
    u.c[0] += i;
    return u.p;
}

// same as p[i & 0xff], but saves one AVR clock cycle when p is 256 byte aligned
template<typename T>
static FORCEINLINE T pagelookup(const void *p, uint8_t i)
{
    return *pageindex<const T>(static_cast<const T*>(p), i);
}

void loadIsinToScratch2();
void loadUsinToScratch3();



#ifdef _MSC_VER
#define _ucosptr (CFG_PTR_USINTAB+64u)
#define _icosptr (CFG_PTR_ISINTAB+64u)
#else
static constexpr const uint8_t * const _ucosptr = CFG_PTR_USINTAB+64u;
static constexpr const uint8_t * const _icosptr = CFG_PTR_ISINTAB+64u;
#endif

#define USIN8FAST(i) pagelookup<uint8_t>(CFG_PTR_USINTAB, uint8_t(i))
#define UCOS8FAST(i) pagelookup<uint8_t>(_ucosptr, uint8_t(i))
#define ISIN8FAST(i) pagelookup<int8_t>(CFG_PTR_ISINTAB, uint8_t(i))
#define ICOS8FAST(i) pagelookup<int8_t>(_icosptr, uint8_t(i))



/*
struct FSinCosScratchFP88
{
    typedef uint8_t angle_type;
    static FORCEINLINE fp88 sin(uint8_t a)
    {
        //return (ISIN8FAST(a) / 127.0f);
        fp88 *p = (fp88*)CFG_PTR_ISINTAB;
        return p[a];
    }
    static FORCEINLINE fp88 cos(uint8_t a)
    {
        //return (ICOS8FAST(a) / 127.0f);
        fp88 *p = (fp88*)CFG_PTR_ISINTAB;
        return p[(a+64) & 0xff];
    }
};
*/

// This is super incorrect, but still gets the job done for rotation matrices etc.
struct FSinCosScratch
{
    typedef uint8_t angle_type;
    static FORCEINLINE fp88 sin(uint8_t a)
    {
        return fp88::raw(ISIN8FAST(a) * uint8_t(2)); // important that sign extension happens
    }
    static FORCEINLINE fp88 cos(uint8_t a)
    {
        return fp88::raw(ICOS8FAST(a) * uint8_t(2));
    }
};

struct FSinCosPROGMEM
{
    typedef uint8_t angle_type;
    static FORCEINLINE fp88 sin(uint8_t a)
    {
        return fp88::raw(isin8slow(a) * uint8_t(2)); // important that sign extension happens
    }
    static FORCEINLINE fp88 cos(uint8_t a)
    {
        return fp88::raw(icos8slow(a) * uint8_t(2));
    }
};

// higher precision by lerp between two consecutive sin/cos values
struct FSmoothSinCosScratch
{
    typedef fp88 angle_type;
    static fp1616 sin(fp88 x);
    static fp1616 cos(fp88 x);
};

