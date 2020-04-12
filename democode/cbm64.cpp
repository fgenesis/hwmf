#include "cbm64.h"
#include "src/demolib/decomp.h"
#include "data.h"

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif

typedef LCD::PixelPos PixelPos;
typedef LCD::ColorType Color;
typedef LCD::DimType DimType;

static void loadbuf(void *dst, fglcd::FarPtr src, unsigned pksz)
{
    decompressRAM<PK_EXO, CBM64Font::size_unpacked, fglcd::ProgmemFar>(dst, src, pksz);
}

void CBM64Font::loadCharset1()
{
    loadbuf(_data, fglcd_get_farptr(data_exo_cbm1), Countof(data_exo_cbm1));
}

void CBM64Font::loadCharset2()
{
    loadbuf(_data, fglcd_get_farptr(data_exo_cbm2), Countof(data_exo_cbm2));
}

void CBM64Font::drawchar(char c, const LCD::ColorType fg, const LCD::ColorType bg) const
{
    typename fglcd::RAM::Pointer fp = _data + uint16_t(uint8_t(c)) * 8u;
    for(uint8_t yy = 0; yy < 8; ++yy)
    {
        uint8_t dat = fglcd::RAM::readIncFast(fp);

        FGLCD_REP_8({
            LCD::sendPixel(dat & 0x80 ? fg : bg);
            dat <<= 1u;
        });
    }
}

LCD::PixelPos CBM64Font::drawbuf(const uint8_t * s, unsigned len, LCD::DimType x, LCD::DimType y, const LCD::ColorType fg, const LCD::ColorType bg) const
{
    const DimType startx = x;
    DimType endy = y + 7;
    while(len--)
    {
        const uint8_t c = fglcd::RAM::readIncFast(s);
        if(UNLIKELY(c == '\n' || x >= LCD::WIDTH-8))
        {
            y += 8;
            endy += 8;
            x = startx;
            if(c == '\n')
                continue;
        }

        LCD::setxy_inl(x, y, x+7, endy);
        drawchar(c, fg, bg);
        x += 8;
    }
    return PixelPos(x,y);
}



#if 0
    template<typename FontMemory, typename StringMemory>
    static NOINLINE void drawfont_1bit_solid_8x8_columnwise(typename FontMemory::Pointer const fontdata, typename StringMemory::Pointer s, DimType x, DimType y, const ColorType fg, const ColorType bg)
    {
        const DimType startx = x;
        u8 c;
        goto begin;
        while((c = StringMemory::template read<u8>(s++)))
        {
            if(c != '\n' && (x+7) < WIDTH)
            {
                const unsigned fontOffs = c * 8;
                for(u8 yy = 0; yy < 8; ++yy)
                {
                    u8 dat = FontMemory::template read<u8>(&fontdata[fontOffs + yy]);
                    for(u8 xx = 0; xx < 8; ++xx, dat >>= 1)
                    {
                        sendPixel(dat & 0x1 ? fg : bg);
                    }
                }
                x += 8;
            }
            else
            {
                y += 8;
                x = startx;
                begin:
                Chip::setxy_inl(y, x, y+7, XMAX);
            }
        }
        return PixelPos(x,y);
    }
#endif
