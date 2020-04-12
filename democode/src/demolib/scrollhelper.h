#pragma once

#include "cfg-demo.h"
#include "../demomath/fgmath.h"

template<unsigned SCROLL_W, unsigned REDRAW_SIZE = 256>
struct WidescrollState
{
    static constexpr uint16_t SCROLL_LCM = LCM<SCROLL_W, LCD::WIDTH>::value;
    static_assert(SCROLL_LCM > 0 && SCROLL_LCM < 0x7fff, "fuck");

    static constexpr uint16_t SCROLL_MOD = LCD::WIDTH < SCROLL_W ? LCD::WIDTH : SCROLL_W;

private:
    static const uint8_t REDRAW_MASK = REDRAW_SIZE-1;
    volatile uint16_t redraw[REDRAW_SIZE];
public:
    FORCEINLINE uint16_t getDrawCol(uint8_t idx) const { return redraw[idx & REDRAW_MASK]; }
    volatile uint8_t wpos;
    int8_t direction;
    uint16_t scrollpos;

    WidescrollState()
        : wpos(0), scrollpos(0)
    {
    }

    void _adjustflip()
    {
        const uint16_t sp = scrollpos - uint16_t(LCD::WIDTH) * oldsgn;
        scrollpos = vmodpos<uint16_t, SCROLL_LCM>(sp);
    }

    void flip() // manual trigger
    {
        _adjustflip();
        direction = -direction;
        oldsgn = -oldsgn;
    }

    int8_t oldsgn;
    void checkflip(int8_t sgn)
    {
        if(sgn)
        {
            if(!(sgn + oldsgn))
                _adjustflip();
            oldsgn = sgn;
        }
    }

    uint16_t scroll(int8_t adv)
    {
        uint8_t a = vabs(adv);
        const int8_t sgn = vsgn(adv);
        checkflip(sgn);
        uint8_t w = wpos;
        //wpos = w + a;
        const uint16_t oldsp = scrollpos;
        uint16_t sp = oldsp;
        while(a--)
        {
            redraw[w++ & REDRAW_MASK] = sp;
            sp += sgn; // not bad if this overflows outside of SCROLL_LCM; the actual draw code does its own modulo
        }
        wpos = w;
        sp = vmodpos<int16_t, SCROLL_LCM>(sp); // prevent wraparound glitching and ensure the subtraction loops in vmodpos() don't take too long
        scrollpos = sp;
        const uint16_t spmod = vmodpos<uint16_t, SCROLL_MOD>(sp);
        return (uint16_t)spmod; // always positive
    }

    uint16_t smooth;
    uint16_t smoothscroll(uint16_t aa)
    {
        uint16_t s = smooth + aa;
        smooth = s & 0xff;
        return scroll(s >> 8);
    }
};
