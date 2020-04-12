// Unfinished test crap goes here

#include "demoshared.h"
#include "rotozoom.h"

demopart part_strobo()
{
    for(;;)
    {
        LCD::clear(0);
        LCD::clear(0xffff);
    }
}

void scrolltest()
{
    LCD::clear(0);
    LCD::set_scroll_area_sides(0, 0);
    //LCD::set_refresh_rate(LCD::HZ_125);

    LCD::fillrect(0, 0, 100, LCD::HEIGHT, 0xffff);

    u8 scroll = 0;
    for(;;)
    {
        LCD::set_scroll_pos(scroll);
        ++scroll;
        //_delay_ms(1);
        LCD::soft_reset();
    }
}

static void evt_reset(void*)
{
    LCD::setColor(0);
    partdone = true;
}

static void timingtest()
{
    // This shows the total number of pixels that can be drawn in 1 ms
    for(;;)
    {
        partdone = false;
        LCD::preparefill();
        LCD::setColor(0xfff);
        evs::schedule(1, evt_reset, NULL);
        LCD::fastfill_inl(LCD::TOTAL_PIXELS);
        while(!partdone) {}
    }
}

demopart test_showwaveform()
{
    partdone = false;
    interp::Interpolator<4> derp(16);

    /*interp::FancyInterpolatedValue<u8> icol;
    derp.add(icol);
    icol.interpolateTo(0xff, 500);*/

    MusicSyncVar<u8> icol(derp, BEATSYNC_CHANNEL1, 0, 255, 3000);

    const u16 maxHalfX = vmin<u16>(CFG_PMFPLAYER_AUDIO_BUFFER_SIZE, LCD::WIDTH / 2) - 2;
    while(!partdone)
    {
        u8 w = icol;
        Color col = LCD::gencolor(w,w,w);
        //printf("%d\n", col);
        for(u16 i = 1; i < maxHalfX; ++i)
        {
            u16 x = i+i;
            LCD::setxywh(x, 0, 1, LCD::YMAX);
            s16 t = music_curSample16(u8(i));
            if(t >= 0)
            {
                LCD::setColor(0);
                LCD::fastfill_u8(64);
                t = vmin<s16>(t, 64);
                const u8 w = (u8)t;
                LCD::setColor(col);
                LCD::fastfill_u8(w);
                LCD::setColor(0);
                LCD::fastfill_u8(64-w);
            }
            else
            {
                t = vmin<s16>(-t, 64);
                const u8 w = (u8)t;
                LCD::setColor(0);
                LCD::fastfill_u8(64-w);
                LCD::setColor(col);
                LCD::fastfill_u8(w);
                LCD::setColor(0);
                LCD::fastfill_u8(64);
            }
        }
    }
}

// Hi :D
// -slerpy

void test_showimage()
{
    DecompImageData<data_pcb64_png> img;
    img.applypal();
    Draw::drawimageRaw<fglcd::RAM>(0, 0, img.w, img.h, img.ptr());
}

void test_rotozoom()
{
    rotozoom::Helper<data_pcb64_png> rz;
    u16 a = 0;
    while(!partdone)
    {
        rz.draw(a);
        a += 55;
    }
}

/*
void partmissing(const char *name_P, u16 ms, bool wait)
{
    u16 oldms = ms;
    DrawFont<data_vgadebug_glf> font(scratch3);
    char name[32];
    strncpy_P(name, name_P, sizeof(name)-1);
    char buf[128];
    LCD::PixelPos pp;
    pp.x = 80;
    pp.y = 80;
    while(!partdone)
    {
        u16 s = ms / 1000;
        u16 rms = ms - (s * 1000);
        snprintf_P(buf, sizeof(buf), PSTR("[%s]\nPart missing (%u ms)!\n%u.%u remain...\n[wait=%u]"),
            name, oldms, s, rms / 100, wait);
        font.template drawStr<fglcd::RAM>(pp, buf, 0xffff, 0);

        fglcd::delay_ms(100);
        ms = vmin<u16>(ms, ms - 100);
        if(!ms && !wait)
            break;
    }
    partdone = false;
}
*/