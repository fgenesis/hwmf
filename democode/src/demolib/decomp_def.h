#pragma once

#include "packdef.h"

enum DecompWindowType
{
    DECOMP_WINDOW_NONE,
    DECOMP_WINDOW_OUTPUT, // use output buffer as window
    DECOMP_WINDOW_BIG, // use internal buffer as 16bit addressable window (but handle wraparound)
    DECOMP_WINDOW_TINY, // use internal 8-bit addressable buffer window (faster, implicit wraparound)
};

enum DecompDst
{
    ToRAM,
    ToLCD,
    ToRLE, // recompress as RLE
};

// these are specialized as needed
template<DecompDst> struct SelectSideEffect;
template<PackType, typename Reader> struct DecompState;
