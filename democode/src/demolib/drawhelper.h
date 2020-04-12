#pragma once

#include "demo_def.h"
#include "fglcd/mcu.h"
#include "scratch.h"


#if CFG_SCRATCH_ALIGNMENT != 256
#error fix this
#endif

#if CFG_PAL_SIZE > 256
#error fix this
#endif

FORCEINLINE static uint16_t palgetcolor_inl(uint8_t b)
{
#ifdef FGLCD_ASM_AVR
    uint16_t c;
    void *x;
    const uint8_t * const pal = CFG_PAL_START_LO;
    // asm version takes advantage of page-alignment of the scratch buffers
    asm volatile(
        "movw %A[x], %[pal]  \n\t"
        "add %A[x], %[b]  \n\t"
        "ld %A[c],%a[x]  \n\t"
#if CFG_PAL_SIZE == 256
        "inc %B[x]  \n\t"
#elif CFG_PAL_SIZE < 256
        "add %A[x],%[offs]  \n\t"
#endif
        "ld %B[c],%a[x]"
        : [c] "=&r" (c), [x] "=&x" (x) /*outputs*/ /* or possibly "=&e" to use any ptr reg */
        : [b] "r" (b), [pal] "r" (pal) /* inputs */
#if CFG_PAL_SIZE < 256
        , [offs] "r" (CFG_PAL_SIZE)
#endif
    );
    return c;
    #else
    return CFG_PAL_START_LO[b] | (CFG_PAL_START_HI[b] << 8u);
    #endif
}

NOINLINE static uint16_t palgetcolor_noinl(uint8_t b)
{
    return palgetcolor_inl(b);
}

void applyPal16_PF(fglcd::FarPtr pal, uint16_t n, uint8_t paloffs = 0);
void applyPal16_RAM(const uint16_t *pal, uint16_t n, uint8_t paloffs = 0);
void palsetcolor(uint8_t b, uint16_t c);
void fuckpal(); // debug shit
void clearpal(Color c);

#define APPLYPAL_PF(pf, offs) applyPal16_PF(fglcd_get_farptr(pf), Countof(pf), (offs));

template<typename TImage, unsigned npal>
struct _PalApplyHelper
{
    static NOINLINE void Apply(uint8_t offs)
    {
        static_assert(TImage::npal == npal, "eh");
        static_assert(TImage::npal + TImage::paloffs < CFG_PAL_SIZE, "pal too large");
        applyPal16_PF(fglcd_get_farptr(TImage::pal), TImage::npal, TImage::paloffs + offs);
    }
};
template<typename TImage>
struct _PalApplyHelper<TImage, 0>
{
    static FORCEINLINE void Apply(uint8_t) {}
};

// also works for meshes
template<typename TImage>
FORCEINLINE void applyPal16_Image(uint8_t offs = 0)
{
    _PalApplyHelper<TImage, TImage::npal>::Apply(offs);
}

// use (2 * shadelevels) + 1 colors for each original color in the mesh.
// assuming 2 original colors A, B, with shadelevels == 2, they will be ordered like this:
// A--, A-, A, A+, A++,   B--, B-, B, B+, B++
// Where +, - means dimmed/brightened color.
// Distance between adjacent colors of the same shade level is [2*shadelevel].
// The first original color is located at index [shadelevel + offset].
void shadepal   (uint8_t shadelevels, uint8_t mul, uint8_t offs, const Color  *base, uint8_t npal);
void shadepal_PF(uint8_t shadelevels, uint8_t mul, uint8_t offs, fglcd::FarPtr base, uint8_t npal);

template<typename TMesh, unsigned npal>
struct _MeshPalApplyHelper
{
    static NOINLINE void Apply(uint8_t shadelevels, uint8_t mul, uint8_t offs)
    {
        static_assert(TMesh::npal == npal, "eh");
        static_assert(TMesh::npal + TMesh::paloffs < CFG_PAL_SIZE, "pal too large");
        shadepal_PF(shadelevels, mul, offs, fglcd_get_farptr(TMesh::pal), TMesh::npal);
    }
};
template<typename TMesh>
struct _MeshPalApplyHelper<TMesh, 0>
{
    static FORCEINLINE void Apply(uint8_t, uint8_t, uint8_t) {}
};

template<typename TMesh>
FORCEINLINE void applyPal16_Mesh(uint8_t shadelevels, uint8_t mul, uint8_t offs)
{
    _MeshPalApplyHelper<TMesh, TMesh::npal>::Apply(shadelevels, mul, offs);
}

struct PalBackup
{
    uint8_t col[CFG_PAL_SIZE * sizeof(Color)];
    PalBackup();
    ~PalBackup();
};

Color dampenColor(Color c, uint8_t sub);
Color brightenColor(Color c, uint8_t add);

void dampenColors(uint8_t start, uint8_t sub, uint8_t n);
void brightenColors(uint8_t start, uint8_t add, uint8_t n);
