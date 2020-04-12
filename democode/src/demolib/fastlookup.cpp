#include "fastlookup.h"
#include "demo_def.h"
#include "fglcd/fglcd.h"
#include "../demomath/fgmath.h"

void loadIsinToScratch2()
{
    fglcd::ProgmemFar::Memcpy(CFG_PTR_ISINTAB, fglcd_get_farptr(lut_isintab), 256);
}

void loadUsinToScratch3()
{
    fglcd::ProgmemFar::Memcpy(CFG_PTR_USINTAB, fglcd_get_farptr(lut_isintab), 256);
    uint8_t *a = (uint8_t*)CFG_PTR_USINTAB;
    uint8_t i = 0;
    do
        *a++ += 127;
    while(++i);
}

fp1616 FSmoothSinCosScratch::sin(fp88 x)
{
    int16_t a = ISIN8FAST(x.intpart()) << 8;
    if(uint8_t m = x.mantissa())
    {
        int16_t b = ISIN8FAST(x.intpart() + 1) << 8;
        a = lerp_inl(a, b, m);
    }
    fp1616 ret = fp1616::raw(int32_t(a) * 2);
    return ret;
}
fp1616 FSmoothSinCosScratch::cos(fp88 x)
{
    int16_t a = ICOS8FAST(x.intpart()) << 8;
    if(uint8_t m = x.mantissa())
    {
        int16_t b = ICOS8FAST(x.intpart() + 1) << 8;
        a = lerp_inl(a, b, m);
    }
    fp1616 ret = fp1616::raw(int32_t(a) * 2);
    return ret;
}


// needs 2 scratch pages
/*
void loadFP88sinToScratch2and3()
{
    fp88 *p = (fp88*)CFG_PTR_ISINTAB;
    uint8_t i = 0;
    do
        *p++ = isin8slow(i) / 127.0f;
    while(++i);
}*/
