#pragma once

#include "demoshared.h"

struct TwistParams
{
    uint8_t xRotMult; // the twist
    uint8_t aRotMult; 
    uint8_t wobbleMult; // how much the thing is a static sinewave
    uint8_t soundWobMult;
    uint8_t baseThickness;
    uint8_t wobThickness;
    uint8_t twistMult; // overall rotation
};

struct TwistState
{
    TwistParams p;
    unsigned a, r;
    uint8_t colorsPerSide;

    struct
    {
         u16 upper[LCD::WIDTH]; // store upper and lower y positions for clearing
         u16 lower[LCD::WIDTH]; 
    } yy;


    TwistState(unsigned a, unsigned r);

    FORCEINLINE void initpal_PF(fglcd::FarPtr p, uint8_t mul, uint8_t colorsPerSide_, uint8_t paloffs = 0)
    {
        shadepal_PF(colorsPerSide_ / 2, mul, paloffs, p, 4);
        colorsPerSide = colorsPerSide_;
    }

    FORCEINLINE void initpal(const Color *p, uint8_t mul, uint8_t colorsPerSide_, uint8_t paloffs = 0)
    {
        shadepal(colorsPerSide_ / 2, mul, paloffs, p, 4);
        colorsPerSide = colorsPerSide_;
    }
};

void drawtwisthoriz(TwistState& s, uint8_t xstart, uint8_t xinc, s8 yoffs = 0, Color bordercol = 0xffff);
