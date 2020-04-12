#include "demoshared.h"

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif

typedef DrawMesh<data_cockpit_obj, DM_STOREZ> TheMesh;

enum Colors
{
    CLOUD1 = LCD::gencolor_inl(0xff, 0xff, 0xff),
    CLOUD2 = LCD::gencolor_inl(0xee, 0xee, 0xee),
    CLOUD3 = LCD::gencolor_inl(0xdd, 0xdd, 0xdd),
    CLOUD4 = LCD::gencolor_inl(0xcc, 0xcc, 0xcc),
    CLOUD5 = LCD::gencolor_inl(0xbb, 0xbb, 0xbb),
    CLOUD6 = LCD::gencolor_inl(0xaa, 0xaa, 0xaa),
    CLOUD7 = LCD::gencolor_inl(0x99, 0x99, 0x99),
    CLOUD8 = LCD::gencolor_inl(0x88, 0x88, 0x88),
    SKY = LCD::gencolor_inl(0x42, 0x80, 0xf1),
    WHAT = LCD::gencolor_inl(0xff, 0, 0),
    LAND1 = LCD::gencolor_inl(0x66, 0xee, 0x44),
    LAND2 = LCD::gencolor_inl(0x44, 0xdd, 0x33),
    LAND3 = LCD::gencolor_inl(0x33, 0xbb, 0x22),
    LAND4 = LCD::gencolor_inl(0x22, 0x99, 0x11),
    LAND5 = LCD::gencolor_inl(0x11, 0x77, 0x00),
};

// heights are carefully measured by examining
enum Limits
{
    TOP_BAND_Y = 0,
    TOP_BAND_H = 46,
    BOTTOM_BAND_H = 52, //44 without LAND5
    BOTTOM_BAND_Y = LCD::YMAX - BOTTOM_BAND_H - 20,

    SPRITE_W = 78,
    SPRITE_H = 50,
};



static FORCEINLINE u8 cloud(unsigned x)
{
    s8 w = ISIN8FAST(x/8) / 4;
    s8 q = ISIN8FAST(x/2 + w) / 2;
    u8 y1 = (USIN8FAST(x + q)) / 16; // usin == 127+isin
    u8 y2 = (USIN8FAST(x + x/2 - q)) / 8;

    return vmax(y1, y2);
}

static FORCEINLINE u8 land(unsigned x)
{
    const u8 y = cloud(~x);
    return hiscale8(s8(y+u8(0x3f)), s8(y+0x1f)); // yeah whatever
}
static FORCEINLINE unsigned scl(unsigned x)
{
    return x >> 4;
}

template<Color col>
static FORCEINLINE void adv(u8& pos, u8 y)
{
    if(pos < y)
    {
        LCD::setColor(col);
        LCD::fastfill_u8(y - pos);
        pos = y;
    }
}

#define TOTEX

demopart part_test_landscape()
{
    partdone = false;
    LCD::clear(SKY);

    loadIsinToScratch2();
    loadUsinToScratch3();
    u8 fb[SPRITE_W * SPRITE_H];

    TheMesh mesh;
    mesh.applypal(11, 7, 0xdf, 0xdf);
    palsetcolor(0xff, SKY);

    Draw::fillrectfromto(0, BOTTOM_BAND_Y + BOTTOM_BAND_H, LCD::XMAX, LCD::YMAX, LAND1);

    unsigned a = 0, b = 554, c = 3372, d = 8773, e = 2231, f = 291, g = 5532;
    unsigned q = 0;
    raster::Params rp;
    rp.align = 1;
    rp.alignoffs = 0;
    //rp.face = 1;
    rp.incr = 1;
    //rp.backfaceoffs = 1;
    rp.rubmul = 0;
    rp.glitchmul = 0;

#ifndef TOTEX
    raster::ClearHelper<2> clr;
#endif

    while(!partdone)
    {
        LCD::set_address_mode(LCD::ADDRMODE_LR_TB); // draw columnwise
        LCD::setxywh_inl(TOP_BAND_Y, 0, TOP_BAND_H, LCD::WIDTH); // freaky x/y-addrmode swap shit

        for(unsigned x = 0; x < LCD::WIDTH; x += 2)
        {
            u8 pos = 0;
            adv<CLOUD2>(pos,    cloud(scl(a)+x));
            adv<CLOUD4>(pos, 10+cloud(scl(b)+x));
            adv<CLOUD6>(pos, 15+cloud(scl(c)+x));
            adv<SKY>(pos, TOP_BAND_H);

            pos = 0;
            adv<CLOUD3>(pos,    cloud(scl(d)+x));
            adv<CLOUD5>(pos, 10+cloud(scl(e)+x));
            adv<CLOUD7>(pos, 15+cloud(scl(f)+x));
            adv<SKY>(pos, TOP_BAND_H);
        }

        LCD::set_address_mode(LCD::ADDRMODE_RL_TB); // flip it so the math can stay as-is
        LCD::setxywh_inl(BOTTOM_BAND_Y, 0, BOTTOM_BAND_H, LCD::WIDTH); // freaky x/y-addrmode swap shit

        for(unsigned x = 0; x < LCD::WIDTH; ++x)
        {
            u8 pos = 0;
            adv<LAND1>(pos,     land(scl(a)+x));
            adv<LAND2>(pos,    cloud(scl(c)+x));
            adv<LAND3>(pos, 10+cloud(scl(d)+x));
            adv<LAND4>(pos, 22+ land(scl(f)+x));
            adv<LAND5>(pos, 30+ land(scl(g)+x));
            adv<SKY>(pos, BOTTOM_BAND_H);
        }

        a += 57;
        b += 49;
        c += 41;
        d += 31;
        e += 25;
        f += 19;
        g += 10;

        // ----- update ship buf ----
        LCD::set_address_mode(LCD::ADDRMODE_LANDSCAPE);

        const unsigned ix = (LCD::WIDTH / 2 - 60) + cloud(q);
        const unsigned iy = (LCD::HEIGHT / 2 - 40) + cloud(q+q/2);

        fglcd::RAM::Memset(fb, 0xff, sizeof(fb)); // sky color
#ifdef TOTEX
        mat4t mt(SPRITE_W / 2 + 2, SPRITE_H / 2, 0);
#else
        mat4t mt(SPRITE_W / 2 + 2 + ix, SPRITE_H / 2 + iy, 0);
#endif
        mat4rx mrx(q++, FSinCosScratch());
        mat4s ms(10);
        const auto m = mt * ms * mrx;
        mesh.transform(m);
        raster::TriFilterResult filt = mesh.filter();
        uint8_t *front = mesh.frontFaceIdxs();
        mesh.sortIndexZ(front, filt.nFront);

#ifdef TOTEX
        mesh.drawToTex(mesh.frontFaceIdxs(), filt.nFront, rp, 0, 0, fb, SPRITE_W, SPRITE_H);
        Draw::drawimageRaw<fglcd::RAM>(ix, iy, SPRITE_W, SPRITE_H, fb);
#else
        clr.add(mesh.getAABB(), 4);
        clr.clear(SKY);
        mesh.draw(mesh.frontFaceIdxs(), filt.nFront, rp, 0, 0);
        //mesh.drawZ_DEBUG(mesh.frontFaceIdxs(), filt.nFront, rp, 0, 0);
        //mesh.drawWireframe(mesh.backFaceIdxs(filt.nBack), filt.nBack, 0);
        //mesh.drawWireframe(mesh.frontFaceIdxs(), filt.nFront, 0xffff);
        //mesh.drawWireframeZ_DEBUG(mesh.frontFaceIdxs(), filt.nFront);
#endif

    
    }

    LCD::set_address_mode(LCD::ADDRMODE_LANDSCAPE);
}
