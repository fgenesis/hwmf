#include "demoshared.h"


struct StretchState
{
    fp1616 stretch, accu;
};

static void makestretch(StretchState& ss, u16 *stretches, unsigned n, fp1616 add)
{
    fp1616 stretch = ss.stretch;
    fp1616 accu = ss.accu;
    const fp1616 m = 0.5f;
    for(u16 y = 0; y < n; ++y)
    {
        stretch += 1;
        stretch -= accu * m;
        stretches[y] = stretch.intpart();
        accu += add;
    }
    ss.stretch = stretch;
    ss.accu = accu;
}

demopart part_megadrivescroll()
{
    DecompImageBlocks<data_tiles_gif> img;
    img.applypal();

    loadIsinToScratch2();
    loadUsinToScratch3(); // decompression clobbers this, so do it after

    u16 stretches[LCD::HEIGHT];
    //makestretch(&stretches[0], LCD::HEIGHT, -2, 3.4f / LCD::HEIGHT);

    u8 scrollspeed[LCD::HEIGHT];
    s8 xshift[LCD::HEIGHT];
    s8 xtileoffs[LCD::HEIGHT];
    {
        {
            StretchState ss;
            ss.accu = 1.5f;
            ss.stretch = 0;
            constexpr u16 what = LCD::HEIGHT / 2;
            makestretch(ss, &stretches[0], LCD::HEIGHT / 2,
                -2.1f / what);
            makestretch(ss, &stretches[LCD::HEIGHT / 2], LCD::HEIGHT / 2,
                2.1f / what);
        }

        u16 halfstretch = vmax(stretches[0], stretches[LCD::HEIGHT-1]) / 2;
        constexpr fp1616 f = 127.0f / LCD::HEIGHT; // upper half wave
        constexpr fp1616 depth = 0.4f;
        fp1616 fa = 0;
        for(unsigned i = 0; i < LCD::HEIGHT; ++i)
        {
            scrollspeed[i] = 20 + (u8)vabs<s16>(halfstretch - stretches[i]) / 4;
            s16 x = (depth * ISIN8FAST(fa.intpart())).intpart();
            fa += f;
            s8 xoffs = 0;
            while(x >= img.blockw)
            {
                x -= img.blockw;
                --xoffs;
            }
            while(x < 0)
            {
                x += img.blockw;
                ++xoffs;
            }
            xshift[i] = (s8)x; //scrollspeed[i];
            xtileoffs[i] = xoffs;
        }

    }

    partdone = 0;
    u16 a = 0xff; // glitches when starting with 0
    const u8 INTERLACE = 4;
    const u8 STAGGER = INTERLACE / 2;
    const u16 BLOCK_HEIGHT = 32;
    const u16 largeblocks = (img.w * img.h) / (img.blockw * 32);
    u8 ymasteroffs = 0;
    while(!partdone)
    {
        constexpr u8 tilecenter = (LCD::WIDTH * 2) / img.blockw;
        u16 tiles[NextPowerOf2<unsigned, tilecenter * 2>::value];
        u8 oldseed = 0xff;

        const u8 gxoffs = a / img.blockw;
        const u8 gxshift = a % img.blockw;

        ymasteroffs = STAGGER - ymasteroffs;

        for(u16 y = ymasteroffs; y < LCD::HEIGHT; y += INTERLACE)
        {
            u16 line = stretches[y] - a;
            u8 seed = line / BLOCK_HEIGHT;

            if(seed != oldseed) // one seed always valid for the drawn height of one block
            {
                oldseed = seed;
                TinyRng8 xrng(seed + 1);
                for(u8 i = 0; i < (u8)Countof(tiles); ++i)
                    tiles[i] = vmodpos<u8, largeblocks>(xrng()) * BLOCK_HEIGHT;
            }

            s16 x = s16(xshift[y]) + s16(gxshift);

            LCD::setxy_inl(0, y, LCD::XMAX, y);

            u16 whichtile = line % BLOCK_HEIGHT;

            u16 i = u16(tilecenter) + xtileoffs[y] - gxoffs;

            if(x >= img.blockw)
            {
                x -= img.blockw;
                --i;
            }
            i %= Countof(tiles); // known to be power of 2 so this is fast

            if(x) // left partial tile? unpack to temp. buffer
            {
                u8 buf[img.blockw];
                img.unpackBlock<ToRAM>(buf, tiles[i] + whichtile);
                u8 ofs = u8(img.blockw) - u8(x);
                Draw::drawimageRaw<fglcd::RAM>(&buf[ofs], x);
            }
            ++i;

            // fill lines directly to LCD
            for( ; x < LCD::WIDTH-img.blockw; ++i, x += img.blockw)
                img.unpackBlock<ToLCD>(NULL, tiles[i] + whichtile);

            if(x < LCD::WIDTH) // right partial tile? unpack to temp. buffer
            {
                u8 buf[img.blockw];
                img.unpackBlock<ToRAM>(buf, tiles[i] + whichtile);
                u8 todo = u8(LCD::WIDTH - x);
                Draw::drawimageRaw<fglcd::RAM>(&buf[0], todo);
            }

#ifdef MCU_IS_PC
            fglcd::delay_ms(1);
#endif
        }
        a += 1;
    }
}
