#include "drawhelper.h"
#include "scratch.h"
#include "../demomath/fgmath.h"

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif


void palsetcolor(uint8_t b, uint16_t c)
{
#ifdef FGLCD_ASM_AVR
    void *x;
    uint8_t *pal = CFG_PAL_START_LO;
    asm volatile(
        "movw %A[x], %[pal]  \n\t"
        "add %A[x], %[b]  \n\t"
        "st %a[x], %A[c]  \n\t"
#if CFG_PAL_SIZE == 256
        "inc %B[x]  \n\t"
#elif CFG_PAL_SIZE < 256
        "add %A[x],%[offs]  \n\t"
#endif
        "st %a[x], %B[c]"
        : [x] "=&x" (x) /*outputs*/
        : [b] "r" (b), [pal] "r" (pal), [c] "r" (c) /* inputs */
#if CFG_PAL_SIZE < 256
        , [offs] "r" (CFG_PAL_SIZE)
#endif
    );
#else
    CFG_PAL_START_LO[b] = ulo8(c);
    CFG_PAL_START_HI[b] = uhi8(c);
#endif
}


void applyPal16_PF(fglcd::FarPtr pal, uint16_t n, uint8_t paloffs)
{
    for(uint16_t i = 0; i < n; ++i)
        palsetcolor(uint8_t(i+paloffs), fglcd::ProgmemFar::template read<uint16_t>(pal, i));
}
void applyPal16_RAM(const uint16_t * pal, uint16_t n, uint8_t paloffs)
{
    for(uint16_t i = 0; i < n; ++i)
        palsetcolor(uint8_t(i+paloffs), fglcd::RAM::template read<uint16_t>(pal, i));
}

void clearpal(Color c)
{
    uint8_t i = 0;
    do
        palsetcolor(i, c);
    while(++i);
}

void fuckpal()
{
    clearpal(LCD::gencolor_inl(0xff, 0, 0xff));
}

void shadepal(uint8_t shadelevels, uint8_t mul, uint8_t offs, const Color *base, uint8_t npal)
{
    if(!shadelevels)
    {
        applyPal16_RAM(base, npal, offs);
        return;
    }
    uint8_t paloffs = shadelevels + offs;
    for(uint8_t z = 0; z < npal; ++z)
    {
        uint8_t c[3];
        LCD::splitcolor(base[z], &c[0]);
        for(uint8_t i = 0; i < shadelevels; ++i)
        {
            uint8_t dark[3], light[3];
            const uint16_t m16 = mul * i;
            const uint8_t m = m16 > 0xff ? 0xff : uint8_t(m16);
            for(uint8_t k = 0; k < 3; ++k)
            {
                //dark[k]  = (uint8_t)vmax<int16_t>(0,    int16_t(c[k]) - m);
                //light[k] = (uint8_t)vmin<int16_t>(0xff, int16_t(c[k]) + m);
                dark[k] = saturateSub<uint8_t>(c[k], m);
                light[k] = saturateAdd<uint8_t>(c[k], m);
            }
            const uint8_t t = (z*shadelevels*2) + paloffs;
            palsetcolor(t-i, LCD::gencolor(dark[0], dark[1], dark[2]));
            palsetcolor(t+i, LCD::gencolor(light[0], light[1], light[2]));
        }
    }

    /*
    union Col3
    {
        uint8_t r,g,b;
        uint8_t a[3];
    };
    uint8_t idx = shadelevels + offs;
    const uint8_t adv = 2 * shadelevels;
    for(uint8_t i = 0; i < npal; ++i, idx += adv)
    {
        const uint16_t col = fglcd::ProgmemFar::read<uint16_t>(pal, i);
        palsetcolor(idx, col);
        Col3 cc;
        LCD::splitcolor(col, cc.a);
        for(uint8_t k = 0; k < shadelevels; ++k)
        {
            for(uint8_t x = 0; x < Countof(cc.a); ++x)
                cc.a[x] = dim8(cc.a[x]);
            palsetcolor(idx-k, LCD::gencolor(cc.r, cc.g, cc.b));
        }
        LCD::splitcolor(col, cc.a);
        for(uint8_t k = 0; k < shadelevels; ++k)
        {
            for(uint8_t x = 0; x < Countof(cc.a); ++x)
                cc.a[x] = brighten8(vmin(cc.a[x], uint8_t(1)));
            palsetcolor(idx+k, LCD::gencolor(cc.r, cc.g, cc.b));
        }
    }
    */
}

void shadepal_PF(uint8_t shadelevels, uint8_t mul, uint8_t offs, fglcd::FarPtr base, uint8_t npal)
{
    Color *tmp = (Color*)StackAlloc(npal * sizeof(Color));
    fglcd::ProgmemFar::Memcpy(tmp, base, npal * sizeof(Color));
    shadepal(shadelevels, mul, offs, tmp, npal);
}

PalBackup::PalBackup()
{
    fglcd::RAM::Memcpy(col, CFG_PAL_START_LO, sizeof(col));
}

PalBackup::~PalBackup()
{
    fglcd::RAM::Memcpy(CFG_PAL_START_LO, col, sizeof(col));
}

Color dampenColor(Color c, uint8_t sub)
{
    uint8_t col[3];
    LCD::splitcolor(c, col);
    for(uint8_t k = 0; k < 3; ++k)
        col[k] = saturateSub(col[k], sub);
    return LCD::gencolor(col[0], col[1], col[2]);
}

Color brightenColor(Color c, uint8_t add)
{
    uint8_t col[3];
    LCD::splitcolor(c, col);
    for(uint8_t k = 0; k < 3; ++k)
        col[k] = saturateAdd(col[k], add);
    return LCD::gencolor(col[0], col[1], col[2]);
}

// assumes backup of the palette right after n colors
void dampenColors(uint8_t start, uint8_t sub, uint8_t n)
{
    const uint8_t end = start + n;
    for(uint8_t i = start; i < end; ++i)
    {
        uint16_t c = palgetcolor_noinl(i + n);
        c = dampenColor(c, sub);
        palsetcolor(i, c);
    }
}

// assumes backup of the palette right after n colors
void brightenColors(uint8_t start, uint8_t add, uint8_t n)
{
    const uint8_t end = start + n;
    for(uint8_t i = start; i < end; ++i)
    {
        uint16_t c = palgetcolor_noinl(i + n);
        c = brightenColor(c, add);
        palsetcolor(i, c);
    }
}
