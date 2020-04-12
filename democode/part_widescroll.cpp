#include "demoshared.h"

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif

typedef DecompStitchedImageBlocks<data_cityscroll1_gif, data_cityscroll2_gif> TheScrollPic;


struct ScrollState : public WidescrollState<TheScrollPic::w>
{
    volatile uint8_t done;
    //volatile uint8_t scrollmode;
    volatile uint8_t rpos;
    uint16_t accu;

    interp::Interpolator<1> inp;
    interp::FancyInterpolatedValue<u8> scrolltimer; // less is faster
};


static void ev_widescroll(void *ud)
{
    fglcd::ProgmemFar::SegmentBackup zz;

    ScrollState& state = *((ScrollState*)ud);
    if(state.done)
    {
        ++state.done;
        return;
    }

    u16 spmod;
    //if(state.scrollmode)
    {
        //const s16 s = ISIN8FAST(state.accu >> 2u);
        constexpr fp1616 fuckingmagicnumber = fp1616(0.111f);
        const fp1616 q = fuckingmagicnumber * state.accu;
        const s16 s = ISIN8FAST(q.intpart());
        spmod = state.smoothscroll(s * 8);
    }
    /*else
    {
        spmod = state.scroll(state.direction);
    }*/
    state.accu += state.direction;

    {
        LCD::StateBackup rti;
        LCD::set_scroll_pos(spmod);
    }
    u16 t = state.scrolltimer;
    evs::schedule(state.scrolltimer, ev_widescroll, ud);
}

static void ev_widescrollstart(void *ud)
{
    ScrollState& state = *((ScrollState*)ud);
    state.scrolltimer.set(24);
    state.scrolltimer.interpolateTo(5, 40);
    state.inp.add(state.scrolltimer);
    state.inp.start(16);
    ev_widescroll(ud);
}

static void dbg_cb(DebugThing& dbg, void *ud, u8 b, u8 bx)
{
    fglcd::ProgmemFar::SegmentBackup zz;

    ScrollState& state = *((ScrollState*)ud);
    /*const s16 sp = state.scrollpos;
    const u16 x = vmodpos<s16, LCD::WIDTH>(sp);
    const u16 ix = vmodpos<s16, TheScrollPic::w>(sp);

    dbg.set2Num(x, ix);*/

    uint8_t w = state.wpos;
    uint8_t r = state.rpos;
    uint8_t d = w-r;
#ifdef __AVR__
    uint8_t z = RAMPZ;
#else
     uint8_t z = 0xff;
#endif

    dbg.set2Num(z, d);

    if(!bx)
        return;

    if(b & bx & 1)
    {
        state.flip();
    }
    if(b & bx & 2)
        --state.direction;
    if(b & bx & 4)
        ++state.direction;
    //if(b & bx & 8)
    //    state.scrollmode = !state.scrollmode;
}

demopart part_widescroll()
{
    loadIsinToScratch2();
    //LCD::set_refresh_rate(LCD::HZ_125);

    TheScrollPic img;
    img.applypal();

    for(u16 x = 0; x < LCD::WIDTH; ++x)
    {
        LCD::setxy(x, 0, x, LCD::YMAX); // one column
        img.toLCD(x);
    }

    ScrollState state;
    fglcd::RAM::Memset((void*)&state, 0, sizeof(state));
    state.scrollpos = LCD::WIDTH;
    state.direction = 1;
    new (&state.inp) interp::Interpolator<1>; // unfortunately the memset fucks this

    DEBUG_THING(dbg_cb, &state);

    evs::schedule(500, ev_widescrollstart, (void*)&state);

    u8 rpos = 0;
    partdone = false;
    while(!partdone)
    {
        const u8 wpos = state.wpos;
        state.rpos = rpos;
        while(rpos != wpos)
        {
            const u16 sp = state.getDrawCol(rpos++);
            const u16 x = vmodpos<u16, LCD::WIDTH>(sp);
            LCD::setxy(x, 0, x, LCD::YMAX); // one column

            const u16 ix = vmodpos<u16, img.w>(sp);
            img.toLCD(ix);
        }
    }
    state.done = 1;

    LCD::clear(0);
    //LCD::set_refresh_rate(LCD::HZ_DEFAULT);
    LCD::set_scroll_pos(0);

    while(state.done == 1) {};
}
