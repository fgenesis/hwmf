#include "demoshared.h"
#include "part_twist.h"

static const u8 BASE_THICKNESS = 111;

struct TwistParamLerp
{
    interp::Interpolator<8> inp;
    MusicSyncVar<u8> beat;
    interp::FancyInterpolatedValue<u8>
        iTwistMult, iRotMult, iWobMult, iSoundWobMult, iWobThick, iWobShrink, iColorSub;

    volatile u8 state, done, threadDone;

    TwistParamLerp()
        : inp(16)
        , beat(inp, BEATSYNC_CHANNEL1, 0, 40, 3000)
        , iRotMult(0), iTwistMult(0), iWobMult(0), iSoundWobMult(0), iWobThick(0), iWobShrink(0)
        , iColorSub(255)
        , state(0)
        , done(0), threadDone(0)
    {
        inp.add(iRotMult);
        inp.add(iTwistMult);
        inp.add(iWobMult);
        inp.add(iSoundWobMult);
        inp.add(iWobThick);
        inp.add(iWobShrink);
        inp.add(iColorSub);
        iColorSub.interpolateTo(0, 500);

        evs::schedule(3000, ev_tick, this);
    }

    ~TwistParamLerp()
    {
        done = 1;
        while(!threadDone) {}
    }

    uint16_t next()
    {
        if(done)
            return 0;
#ifdef MCU_IS_PC
        printf("twist state = %u\n", state);
#endif

        /* progression:
        - start flat
        - rotate all across
        - start to twist
        - start to wobble
        - mix in music
        */
        switch(state++)
        {
            case 0:
                iWobThick.interpolateTo(240, 1200);
                iWobShrink.interpolateTo(BASE_THICKNESS - 30, 500);
                return 2000;
            case 1:
                iWobThick.interpolateTo(0, 600);
                iWobShrink.interpolateTo(0, 600);
                return 1500;
            case 2:
                iRotMult.interpolateTo(150, 300);
                return 2000;
            case 3:
                iTwistMult.interpolateTo(0xff, 100);
                return 2000;
            case 4:
                iWobMult.interpolateTo(100, 800);
                return 2000;
            case 5:
                iSoundWobMult.interpolateTo(64, 300);
                return 2000;
        }

#ifdef MCU_IS_PC
        LCD::fillrectfromto(0,0,30,30, LCD::gencolor_inl(0xff,0,0xff));
#endif
        return 0;
    }

    static void ev_tick(void *ud)
    {
        TwistParamLerp *tpl = (TwistParamLerp*)ud;
        uint16_t t = tpl->next();
        if(t)
            evs::schedule(t, ev_tick, ud);
        else
            tpl->threadDone = 1;
    }

    void apply(TwistParams& p)
    {
        p.xRotMult = iTwistMult;
        p.aRotMult = iRotMult;
        p.wobbleMult = iWobMult;
        p.soundWobMult = iSoundWobMult;
        p.baseThickness = BASE_THICKNESS + beat - iWobShrink;
        p.wobThickness = iWobThick;
        //p.twistMult = iRotMult;
    }
};

static void startparams(TwistParams& p)
{
    p.xRotMult = 0;
    p.aRotMult = 0;
    p.wobbleMult = 0;
    p.soundWobMult = 0;
    p.baseThickness = BASE_THICKNESS;
    p.wobThickness = 0;
    p.twistMult = 64;
}

static void testparams(TwistParams& p)
{
    /*
    p.xRotMult = 0xff;
    p.aRotMult = 0xff;
    p.wobbleMult = 100;
    p.soundWobMult = 64;
    p.baseThickness = 127;
    p.wobThickness = 0;
    p.twistMult = 64;
    */

    p.xRotMult = 0xff;
    p.aRotMult = 0xff;
    p.wobbleMult = 100;
    p.soundWobMult = 0;
    p.baseThickness = 127;
    p.wobThickness = 0;
    p.twistMult = 64;
}



TwistState::TwistState(unsigned a_, unsigned r_)
    : a(a_), r(r_)

{
    startparams(p);
    fglcd::RAM::Memset(&yy, 0, sizeof(yy));
}

struct DrawLineInfo
{
    u16 sx;
    u8 n, pal, angle;
};

static void drawside(const DrawLineInfo& dr, s16 s, u8 colorsPerSide)
{
    u8 k = dr.n; // full length of side to draw, in screen space

    const u8 palidx = dr.angle >> 2;
    const u8 palbase = colorsPerSide*dr.pal;
    const Color bgcol = palgetcolor_inl(palbase + palidx);

    if(!s)
    {
        LCD::setColor(bgcol);
        LCD::fastfill_u8(k);
        return;
    }

    const Color wavcol = palgetcolor_inl(palbase + vmax<s8>(((palidx>>1) - 8), 1));

    const u8 half = k / 2u; // half side
    k -= half; // k is now the maximum amplitude reachable

    /*k                h  
      a   ##           h    
      a #######        h    
      a ------##-----  b 
      h         #####  b 
      h           ##   k
      h            #   k
    */

    if(s > 0)
    {
        // upper wave
        u8 a = uhi8(dr.angle * s);
        if(a < k)
        {
            k -= a; // k is now space between wave and amplitude
            LCD::setColor(bgcol);
            LCD::fastfill_u8(k);
        }
        else
            a = half;
        LCD::setColor(wavcol);
        LCD::fastfill_u8(a);
    }

    LCD::setColor(bgcol); // the empty half
    LCD::fastfill_u8(half);

    if(s < 0)
    {
        // lower wave
        u8 b = uhi8(dr.angle * -s);
        if(b < k)
        {
            k -= b;
            LCD::setColor(wavcol); // lower half
            LCD::fastfill_u8(b);
        }
        else
            b = half;

        LCD::setColor(bgcol);
        LCD::fastfill_u8(k);
    }
}

#define CPS(a, b) if(b.sx < a.sx) vswap(a, b)

void drawtwisthoriz(TwistState& ts, uint8_t xstart, uint8_t xinc, s8 yoffs, Color bordercol)
{
    u16 r = ts.r;
    const u16 a = (u32(ts.a) * ts.p.twistMult) >> 8;
    const u16 wobval = a+a/2;
    const u8 colorsPerSide = ts.colorsPerSide;
    u8 mm = 0;

    s16 lastsample = music_curSample16(mm);
    for(u16 x = xstart; x < LCD::WIDTH; x += xinc, ++r, ++mm)
    {
        // MUSICSYNC
        s16 s = music_curSample16(mm);

        // very rudimentary sample smoothing
        s16 ls = lastsample;
        lastsample = s;
        s += ls;
        s >>= 1;

        // no idea wtf this does but this looks good enough
        const u8 rh = uhi8(r);
        s16 xfactor = ISIN8FAST(rh + x/3 + ICOS8FAST(rh));
        xfactor -= ICOS8FAST((x/2)+rh)/2;
        xfactor -= ISIN8FAST(rh-u8(a/2));
        
        s16 t = hiscale8((u8)a, ts.p.aRotMult);
        t += (xfactor * ts.p.xRotMult) >> 8;
        const s8 q1 = ISIN8FAST((u8)t);
        const s8 q2 = ICOS8FAST((u8)t);
        u8 bt = ts.p.baseThickness;
        if(u8 wt = ts.p.wobThickness)
            bt += uhi8(vabs(s) * ts.p.wobThickness);
        const s8 w1 = hiscale8(q1, bt);
        const s8 w2 = hiscale8(q2, bt);
        const u16 pos = u16(LCD::HEIGHT-(w1+w2))/2;
        DrawLineInfo dr[2];
        u8 dri = 0;
        if(w1 > 0)
            dr[dri++] = {u16(pos), (u8)w1, 0, (u8)q1 };
        if(w2 > 0)
            dr[dri++] = { u16(pos+w1), (u8)w2, 1, (u8)q2 };
        if(w2 < 0)
            dr[dri++] = { u16(pos+w2), (u8)-w2, 2, (u8)-q2 };
        if(w1 < 0)
            dr[dri++] = { u16(pos+w2+w1), (u8)-w1, 3, (u8)-q1};

        if(dri > 1)
            CPS(dr[0], dr[1]);

        u16 y = yoffs + dr[0].sx;
        if(u8 wobble = ts.p.wobbleMult)
        {
            s8 morewob = uhi8(s * ts.p.soundWobMult);
            incrhiscale(y, ICOS8FAST(wobval+x/2+morewob), wobble);
        }
        if(y > LCD::HEIGHT) // underflow?
            y = 0;
        const u16 prevy = ts.yy.upper[mm];
        ts.yy.upper[mm] = y;

        if(prevy < y)
        {
            // erase the old crap
            LCD::setxy(x, prevy, x, LCD::YMAX);
            LCD::setColor(0);
            LCD::fastfill_u8(y - prevy);
        }
        else
            LCD::setxy(x, y, x, LCD::YMAX);

        // --- first side ---

        LCD::sendPixel(bordercol); LCD::sendPixel();
        y += 2;

        drawside(dr[0], s, colorsPerSide);
        y += dr[0].n;

        // --- second side, if visible ---
        if(dri > 1)
        {
            LCD::sendPixel(bordercol); LCD::sendPixel();
            y += 2;

            drawside(dr[1], s, colorsPerSide);
            y += dr[1].n;
        }

        LCD::sendPixel(bordercol); LCD::sendPixel();
        y += 2;

        u16 oldyend = ts.yy.lower[mm];
        ts.yy.lower[mm] = y;
        if(y < oldyend)
        {
            // erase the rest
            LCD::setColor(0);
            u8 d = oldyend - y + 2; // HACK: +2 fixes stray pixels
            LCD::fastfill_u8(d);
        }
    }
    ts.r = r;
}

const Color twistpal_PF[] PROGMEM_FAR = { LCD::gencolor(0xff, 0, 0), LCD::gencolor(0, 0xff, 0), LCD::gencolor(0, 0, 0xff), LCD::gencolor(0xff, 0, 0xff) };


void part_twist()
{
    loadIsinToScratch2();
    loadUsinToScratch3();

    TwistParamLerp twl;

    auto twistpal = farload(twistpal_PF);

   // LCD::set_refresh_rate(LCD::HZ_125);
    TwistState s {0, 0};
    u8 lastColorSub = 0;
    
    partdone = false;

    bool exiting = false;

    const u8 maxHalfX = vmin<u16>(CFG_PMFPLAYER_AUDIO_BUFFER_SIZE, LCD::WIDTH / 2) - 2;
    const u8 BHALF = 90;
    while(true)
    {
        u8 sub = twl.iColorSub;
        if(exiting && sub==255)
            break;
        if(sub != lastColorSub)
        {
            lastColorSub = sub;

            Color tmp[Countof(twistpal_PF)];
            for(u8 i = 0; i < Countof(twistpal_PF); ++i)
                tmp[i] = dampenColor(twistpal[i], sub);
            s.initpal(&tmp[0], (0xff-sub) / 25, 32);
        }
        if(partdone)
        {
            if(!exiting)
                twl.iColorSub.interpolateTo(255, 2000);
            exiting = true;
        }
        twl.apply(s.p);

        ++s.a;
        drawtwisthoriz(s, 0, 2);
    }
   // LCD::set_refresh_rate(LCD::HZ_DEFAULT);

    partdone = false;
}
