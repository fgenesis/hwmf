#pragma once

#include "demoshared.h"

namespace rotozoom {

static FORCEINLINE fp1616 sinval(u16 a)
{
    return FSmoothSinCosScratch::sin(fp88::raw(a));
}

static FORCEINLINE fp1616 cosval(u16 a)
{
    return FSmoothSinCosScratch::cos(fp88::raw(a));
}

#define ROTO_SIZE 6 // also adjust the FGLCD_REP_xx below to whatever this is

template<typename ImageData>
static FORCEINLINE void draw_vert(typename ImageData::Mem::Pointer image, u16 a)
{
    constexpr u16 W = ImageData::w;
    constexpr u16 H = ImageData::h;
    typedef typename ImageData::Mem Mem;

    constexpr fp1616 zoommult = fp1616(0.9f);
    constexpr fp1616 sizemult = fp1616(0.4f);
    constexpr fp1616 panmult = fp1616(32);
    const u16 a3 = a * 3;

    const fp1616 dy = panmult * sinval(a + a3);
    const fp1616 dx = panmult * cosval(a*2);
    vec2 p(dx, dy);

    const fp1616 s = sinval(a) * sizemult - sinval(a3) * zoommult;
    const fp1616 c = cosval(a) * sizemult - cosval(a3) * zoommult;
    const svec2 step(c.tofp88(), s.tofp88());

    for(u16 x = 0; x < LCD::WIDTH; x += ROTO_SIZE)
    {
        svec2 pp(p.x.tofp88(), p.y.tofp88());
        LCD::setxy(x, 0, x, LCD::YMAX);
        for(u8 x = 0; x < LCD::HEIGHT/ROTO_SIZE; ++x, pp += step)
        {
            const u8 xx = pp.x.intpart();
            const u8 yy = pp.y.intpart();
            const u16 datapos = (yy & (H-1)) * W + (xx & u8(W-1));
            const u8 idx = Mem::template read<u8>(image + datapos);
            LCD::setColor(palgetcolor_inl(idx));
            FGLCD_REP_6(LCD::sendPixel());
        }
        p.x += s;
        p.y -= c;
    }
}

template<typename TImage>
struct Helper
{
    typedef DecompImageData<TImage> ImageData;
    ImageData img;

    Helper()
    {
        loadIsinToScratch2();
        applyPal16_Image<TImage>();
    }

    void draw(u16 a)
    {
        draw_vert<ImageData>(img.ptr(), a);
    }
};

} // end namespace rotozoom
