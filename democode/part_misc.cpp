#include "demoshared.h"
#include "part_c64.h"

static void drawchess(const u32 abig, const u16 ystart, const Color fg, const u8 yinc, const u8 wobble)
{
    const u32 asmall = abig >> 2;
    const u16 a = asmall >> 4;
    ivec2 p(int(ISIN8FAST(a/3)*64), int(ICOS8FAST(a/2)*64));
    const s16 s = ISIN8FAST(a);
    const s16 c = ICOS8FAST(a);
    ivec2 step(c, s);

    for(u16 y = AREA_Y_BEGIN + ystart; y < AREA_Y_END; y += yinc)
    {
        ivec2 pp = p;
        if(wobble)
        {
            pp.x += 3*scale8(ISIN8FAST(asmall/4+y), wobble);
            pp.y += 4*scale8(ICOS8FAST(y-asmall/3), wobble);
        }
        
        LCD::setxy(AREA_X_BEGIN, y, AREA_X_END, y);
        for(u8 x = 0; x < AREA_W/4; ++x, pp += step)
        {
            const u8 SIZE = 0x08;
            LCD::setColor(((uhi8(pp.x) & SIZE) ^ (uhi8(pp.y) & SIZE)) == 0 ? 0 : fg);
            FGLCD_REP_4(LCD::sendPixel());
        }
        p.x -= s;
        p.y += c;
    }
}

struct ChessState
{
    ChessState()
        : inp(16)
        , beatsync {
              MusicSyncVar<u8>(inp, BEATSYNC_CHANNEL1, 0, 255, 4000)
            , MusicSyncVar<u8>(inp, BEATSYNC_CHANNEL2, 0, 255, 4000)
            , MusicSyncVar<u8>(inp, BEATSYNC_CHANNEL3, 0, 255, 4000)
        }
        , wobmult(0)
        , clearswipe(0)
    {
        done = 0;
        inp.add(wobmult);
        for(u8 i = 0; i < 3; ++i)
        {
            fadein[i].set(0);
            inp.add(fadein[i]);
        }
        inp.add(clearswipe);
    }

    ~ChessState()
    {
        if(done != 2)
        {
            done = 1;
            while(done != 2) {}
        }
    }

    interp::Interpolator<1+3+3+1> inp;
    MusicSyncVar<u8> beatsync[3];
    interp::FancyInterpolatedValue<u8> wobmult;
    interp::FancyInterpolatedValue<u8> fadein[3];
    interp::FancyInterpolatedValue<u16> clearswipe;
    u8vec3 colors[3];
    u16 incs[3];
    u32 accu[3];
    u8 speedmult;
    u8 beginlayer;
    //u8 layers;
    volatile u8 stage;
    volatile u8 stx;
    volatile u8 done;
};

#define BRIGHT 133

static void drawchesslayers(ChessState& cs)
{
    u8 i = cs.beginlayer;
    const u8 m = i; //+ cs.layers;
    const u8 wobm = cs.wobmult;
    u8 active = 0;
    for(; i < 3; ++i)
    {
        u8 fade = cs.fadein[i];
        if(!fade)
            continue;
        ++active;
        u8 a = scale8(u8(BRIGHT), fade);
        u8vec3 cv = lerp(u8vec3(0), cs.colors[i], fade);
        u8vec3 cmax = u8vec3(
            saturateAdd(cv.r, a),
            saturateAdd(cv.g, a),
            saturateAdd(cv.b, a)
        );
        u8 beat = cs.beatsync[i];
        const Color col = gencolor(lerp(cv, cmax, beat));
        u8 wob = wobm;
        if(wob)
            wob = scale8(beat, wob);
        drawchess(cs.accu[i], i, col, 4, wob);
        cs.accu[i] += cs.incs[i] * cs.speedmult;
    }
    cs.speedmult = active;
}

static void fadein(interp::FancyInterpolatedValue<u8>& v)
{
    v.interpolateTo(255, 2500);
}

static void fadeout(interp::FancyInterpolatedValue<u8>& v)
{
    v.interpolateTo(0, 2500);
}

static void evt_chess(void *ud)
{
    ChessState& cs = *static_cast<ChessState*>(ud);
    if(cs.done)
    {
        cs.done = 2;
        return;
    }
    u16 nexttime = 0;
    u8 stage = cs.stage;
    switch(stage)
    {
        case 0: nexttime = 2600;
            cs.wobmult.interpolateTo(255, 600);
            break;
        case 1: nexttime = 1800;
            cs.colors[0] = u8vec3(0xdf, 0, 0);
            cs.colors[1] = u8vec3(0x3f, 0xdf, 0xdf);
            fadein(cs.fadein[1]);
            cs.accu[1] = cs.accu[0];
            break;
        case 2: nexttime = 3200;
            cs.colors[1] = u8vec3(0, 0, 0xdf);
            cs.colors[2] = u8vec3(0x3f, 0xdf, 0x3f);
            fadein(cs.fadein[2]);
            cs.accu[2] = cs.accu[1];
            break;
        case 3: nexttime = 1600;
            fadeout(cs.fadein[2]);
            break;
        case 4:
            fadeout(cs.fadein[1]);
            break;
    }
    cs.stage = stage + 1;
    if(nexttime)
        evs::schedule(nexttime, evt_chess, ud);
    else
        cs.done = 2;
}

static void evt_chessStart(void *ud)
{
    ChessState& cs = *static_cast<ChessState*>(ud);
    cs.clearswipe.interpolateTo(AREA_W / 2, 900);
    evt_chess(ud);
}

static NOINLINE void swipecol(u16 x)
{
    LCD::setxy(x, AREA_Y_BEGIN, x, AREA_Y_END);
    LCD::setColor(0);
    LCD::fastfill_u16(AREA_H);
}

demopart part_layerchess()
{
    loadIsinToScratch2();

    ChessState cs;
    cs.stage = 0;
    cs.stx = 0;
    //cs.layers = 1;
    cs.beginlayer = 0;
    cs.speedmult = 1;
    cs.accu[0] = 666;
    cs.colors[0] = u8vec3(0xac);
    cs.incs[0] = 0xf<<2;
    cs.incs[1] = 0xe<<2;
    cs.incs[2] = 0xd<<2;
    cs.incs[3] = 0xc<<2;
    fadein(cs.fadein[0]);

    /*cs.coltab[0] = LCD::gencolor(0xff, 0, 0);
    cs.coltab[1] = LCD::gencolor( 0,  0xff, 0);
    cs.coltab[2] = LCD::gencolor(0, 0, 0xff);
    cs.coltab[3] = LCD::gencolor(0x7f, 0x7f, 0x7f);*/

    evs::schedule(1500, evt_chessStart, &cs);
    partdone = false;
    u8 exit = false;
    u8 xswipe = 0;
    while(true)
    {
        const u16 xtarget = cs.clearswipe;
        if(xswipe < xtarget)
        {
            for(u16 x = xswipe; x <= xtarget; ++x)
            {
                swipecol(AREA_X_BEGIN + AREA_W/2 + x);
                swipecol(AREA_X_BEGIN + AREA_W/2 - x);
            }

            xswipe = xtarget;
        }

        drawchesslayers(cs);
        if(exit)
        {
            if(!cs.fadein[0])
                break;
        }
        else if(partdone)
        {
            exit = true;
            for(u8 i = 0; i < 3; ++i)
                fadeout(cs.fadein[i]);
        }
    }
    partdone = false;
}
