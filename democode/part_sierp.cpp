#include "src/demolib/scrollhelper.h"
#include "demoshared.h"
#include "part_c64.h"

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif

static const u8 SierpSize = AREA_H / 4 / 8;

static const u16 USEAREAWIDTH = AREA_W+1;

//static const u8 AUDIOFOO = pmfplayer_max_channels < SierpSize ? pmfplayer_max_channels : SierpSize;

//static const u8 USECHANNELS = 4;

//static_assert(USECHANNELS <= AUDIOFOO, "aa");
//static_assert(USECHANNELS*2 <= SierpSize, "aa");

struct SierpScrollState : public WidescrollState<USEAREAWIDTH, 16>
{
    volatile u8 done;
    volatile u8 delay;
    u16 accu;
};



static NOINLINE void updatesierp(uint8_t *p)
{
#ifdef __AVR__
    // implements n ^= 2*n
    asm volatile(
        "clc  \n\t"
        "byteloop_%=: \n\t"
            "ld r1, %a[p]  \n\t"
            "mov __tmp_reg__, r1  \n\t"
            "rol __tmp_reg__  \n\t"  // carry is needed in the next iteration
            "eor r1, __tmp_reg__  \n\t"
            "st %a[p]+, r1  \n\t"
            "dec %[N]  \n\t"
        "brne byteloop_%=  \n\t"
        "clr r1  \n\t"
        : /* out */
        : [N] "r" (SierpSize), [p] "x" (p)  /* in */
        : "r0"
    );
#else
    // Shitty and slow
    u8 carry = 0;
    for(u8 i = 0; i < SierpSize; ++i)
    {
        u8 v = p[i];
        u16 tmp = u16(v) << 1;
        tmp += carry;
        p[i] = u8(tmp) ^ v;
        carry = (tmp >> 8u) & 1;
    }
#endif
}

static void drawsierpcol(const u8 * const p, u16 x, const u8 coloffs)
{
    x += AREA_X_BEGIN;
    LCD::setxy_inl(x, AREA_Y_BEGIN, x, AREA_Y_END);
    for(u8 k = 1; k < SierpSize; ++k)
    {
        u8 c = p[k];
        for(u8 i = 0; i < 8; ++i)
        {
            const u8 b = c & 1;
            c >>= u8(1u);
            LCD::sendPixel(palgetcolor_inl(b + coloffs));
            LCD::sendPixel();
            LCD::sendPixel();
            LCD::sendPixel();
        }
    }
}

struct SierpUnit
{
    u8 sierp[SierpSize];
    fp1616 shift;
    u16 oldshift;
    const u8 id;

    SierpUnit(u8 id)
        : shift(0), oldshift(0), id(id)
    {
    }

    void reset()
    {
        fglcd::RAM::Memset(sierp, 0, sizeof(sierp));
        sierp[0] = 1;
    }

    void update(fp1616 add)
    {
        u16 sh = shift.intpart();
        shift += add;
        shift.f.part.hi = vmodpos<u16, USEAREAWIDTH>(shift.f.part.hi);
        sh &= ~3;
        if(sh == oldshift)
            return;
        oldshift = sh;
        sh += id;
        u8 color = id << 1;
        for(u16 xx = sh; xx < USEAREAWIDTH; xx += 4)
        {
            updatesierp(sierp);
            drawsierpcol(sierp, xx, color);
        }
        for(u16 xx = id; xx < sh; xx += 4)
        {
            updatesierp(sierp);
            drawsierpcol(sierp, xx, color);
        }

    }
};

struct SierpState
{
    SierpUnit s2, s3, s4;
    SierpScrollState scroll;

    SierpState() : s2(1), s3(2), s4(3)
    {
        fglcd::RAM::Memset(&scroll, 0, sizeof(scroll));
        scroll.scrollpos = LCD::WIDTH;
        scroll.direction = 4;
        scroll.delay = 45;
    }
};

static void dbg_cb(DebugThing& dbg, void *ud, u8 b, u8 bx)
{
    SierpState& ss = *((SierpState*)ud);

    dbg.set2Num(ss.scroll.direction, ss.scroll.delay);

    if(b & bx & 1)
        ss.scroll.flip();
    if(b & bx & 2)
        --ss.scroll.direction;
    if(b & bx & 4)
        ++ss.scroll.direction;
    if(b & bx & 8)
        --ss.scroll.delay;
    if(b & bx & 16)
        ++ss.scroll.delay;
}

static void ev_scroll(void *ud)
{
    SierpState& ss = *((SierpState*)ud);
    if(ss.scroll.done)
    {
        ++ss.scroll.done;
        return;
    }

    u16 spmod = ss.scroll.scroll(ss.scroll.direction);
    ss.scroll.accu += ss.scroll.direction;

    {
        LCD::StateBackup rti;
        FGLCD_ASSERT_VAL(spmod < USEAREAWIDTH, "sierpspm", spmod);
        spmod += AREA_X_BEGIN;
        LCD::set_scroll_pos(spmod);
    }

    evs::schedule(ss.scroll.delay, ev_scroll, ud);
}

static const Color sierppal_PF[] PROGMEM_FAR =
{
    LCD::gencolor_inl(0xff, 0xbf, 0),
    LCD::gencolor_inl(0x56, 0x69, 0xdf),
    LCD::gencolor_inl(0x36, 0x69, 0x2f),
    LCD::gencolor_inl(0x86, 0x39, 0x9f),
};

static void sierpsync(uint8_t channel, pmf_channel_info info, void *ud)
{
    SierpState *state = (SierpState*)ud;
}

void part_sierpscroll()
{
    clearpal(0);
    LCD::set_scroll_pos(AREA_X_BEGIN);

    constexpr unsigned areaw =
    LCD::CONST_scroll_area_sides(AREA_X_BEGIN, LCD::XMAX-AREA_X_END);
    LCD::  set_scroll_area_sides(AREA_X_BEGIN, LCD::XMAX-AREA_X_END);
    static_assert(areaw == AREA_W, "areawwtf");

    //loadIsinToScratch2();
    //loadUsinToScratch3();
    //LCD::set_refresh_rate(LCD::HZ_125);

    u8 sierp1[SierpSize];
    SierpState ss;
    //u8 sierp2base[SierpSize];
    fglcd::RAM::Memset(sierp1, 0, sizeof(sierp1));
    //fglcd::RAM::Memset(sierp2base, 0, sizeof(sierp2base));
    sierp1[0] = 1;
    //sierp2base[0] = 1;
    DEBUG_THING(dbg_cb, &ss);

    //musync::Override mu(BEATSYNC_CHANNEL1, sierpsync, &ss, musync::CallOnHit);

    /*u8 cx = 0;
    do
    {
        palsetcolor(cx, 0);
        palsetcolor(cx+1, 0xffff);
    }
    while(cx += 2);*/

    const u8 NP = Countof(sierppal_PF);
    u8vec3 palv[NP], palvbright[NP];
    {
        auto pal = farload(sierppal_PF);
        palsetcolor(1, pal[0]);
        for(u8 i = 0; i < NP; ++i)
        {
            u8 v[3];
            LCD::splitcolor(pal[i+1], &v[0]);
            for(u8 k = 0; k < 3; ++k)
            {
                palvbright[i][k] = saturateAdd(v[k], 90);
                palv[i][k] = saturateSub(v[k], 30);
            }
        }
    }

    interp::Interpolator<4> inp(16);
    MusicSyncVar<u8vec3> beat1(inp, BEATSYNC_CHANNEL1, palv[0], palvbright[0], 3333);
    MusicSyncVar<u8vec3> beat2(inp, BEATSYNC_CHANNEL2, palv[1], palvbright[1], 3333);
    MusicSyncVar<u8vec3> beat3(inp, BEATSYNC_CHANNEL3, palv[2], palvbright[2], 3333);
    interp::FancyInterpolatedValue<u8> fadein(0);
    inp.add(fadein);
    fadein.interpolateTo(255, 1000);
    evs::schedule(100, ev_scroll, (void*)&ss);

    partdone = false;
    u8 rpos = 0;
    u16 shift2 = 0;
    fp1616 shift3 = 108, shift4 = 44;
    while(!partdone)
    {
        const u8 fade = fadein;
        palsetcolor(3, gencolorScaled(beat1, fade));
        palsetcolor(5, gencolorScaled(beat2, fade));
        palsetcolor(7, gencolorScaled(beat3, fade));

        while(rpos != ss.scroll.wpos)
        {
            const s16 sp = ss.scroll.getDrawCol(rpos++);
            if((sp & 3) == 0)
            {
                const u16 x = vmodpos<s16, USEAREAWIDTH>(sp);
                updatesierp(sierp1);
                drawsierpcol(sierp1, x, 0);
            }
        }

        LCD::set_address_mode(LCD::ADDRMODE_BT_RL);
        ss.s2.reset();
        ss.s2.update(0.5f*4);

        LCD::set_address_mode(LCD::ADDRMODE_LANDSCAPE);
        ss.s3.reset();
        ss.s3.update(2*3.3f);

        LCD::set_address_mode(LCD::ADDRMODE_BT_RL);
        ss.s4.reset();
        ss.s4.update(2*2.6f);

        LCD::set_address_mode(LCD::ADDRMODE_LANDSCAPE);

    }
    ss.scroll.done = 1;
    while(ss.scroll.done == 1) {};
    //LCD::set_refresh_rate(LCD::HZ_DEFAULT);

    LCD::set_scroll_area_sides(0, 0);
    LCD::set_scroll_pos(0);
}
