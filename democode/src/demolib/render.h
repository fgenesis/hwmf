// Rendering helpers used to specialize rasterizer

#pragma once

#include "cfg-demo.h"
#include "../demomath/fgmath.h"
#include <limits.h>
#include "fastlookup.h" // for ISIN8FAST
#include "drawhelper.h"
#include "raster.h"


namespace render {


static FORCEINLINE void drawScanlineLCD(int x0, int x1, unsigned y, LCD::ColorType col)
{
    x0 = vclamp<int>(x0, 0, LCD::XMAX);
    x1 = vclamp<int>(x1, 0, LCD::XMAX);
    
    LCD::setxy(x0, y, x1, y);
    LCD::sendPixel(col); // x0, x1 are inclusive so we'all always send at least one pixel
    LCD::fastfill_u16(x1 - x0); // fill the remaining scanline with the color just set
}

struct ToLCD
{
    ToLCD(uint8_t rub, uint8_t glitch)
        : rubmul(rub), glitchmul(glitch)
    {}
    typedef LCD::ColorType ColorType;
    FORCEINLINE static ColorType getColor(uint8_t id) { return palgetcolor_noinl(id); };
    FORCEINLINE void drawScanline(int x0, int x1, int y, ColorType col) const
    {
        // yeah yeah i know this is a cheap, lame fake and no real rubber. deal with it.
        if(rubmul)
        { 
            int8_t rub = int8_t(y);
            if(glitchmul)
                incrhiscale(rub, uint8_t(uint8_t(x0) ^ uint8_t(x1)), glitchmul);
            rub = hiscale8(ISIN8FAST(rub), (int8_t)rubmul);
            x0 += rub;
            x1 += rub;
        }
        drawScanlineLCD(x0, x1, y, col);
    }
    FORCEINLINE static int height() { return LCD::HEIGHT; }
    const uint8_t rubmul, glitchmul;
};

struct ToTex
{
    ToTex(uint8_t *tex, uint8_t w, uint8_t h)
        : _tex(tex), _w(w), _h(h)
    {}
    FORCEINLINE int height() const { return _h; }
    typedef uint8_t ColorType; // pass-through color ID
    FORCEINLINE static uint8_t getColor(uint8_t id) { return id; };
    FORCEINLINE void drawScanline(int x0, int x1, int y, ColorType col)
    {
        const uint8_t xx0 = (uint8_t)vclamp<int>(x0, 0, _w);
        const uint8_t xx1 = (uint8_t)vclamp<int>(x1, 0, _w);
        fglcd::RAM::Memset(&_tex[uint16_t(y) * _w + xx0], col, xx1-xx0+1);
    }

    uint8_t * const _tex;
    const uint8_t _w, _h;
};

} // end namespace render
