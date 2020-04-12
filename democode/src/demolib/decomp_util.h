#pragma once

#include "fglcd/mcu.h"
#include "demo_def.h"
#include "scratch.h"

namespace decomp {

template<typename Mem, unsigned char bit>
static FORCEINLINE uint16_t readNRaw(uint8_t ctrl, typename Mem::Pointer& p)
{
    static const uint8_t MORE = (1 << bit);
    static const uint8_t MASK = MORE - 1;
    uint8_t n = ctrl & MASK;
    if(!(ctrl & MORE))
        return n; // highest bit is never set, so this can't overflow

    const uint8_t lo = Mem::readIncSafe(p);
    return fglcd::make16(lo, n);
}

template<typename Mem>
static FORCEINLINE uint16_t readN(typename Mem::Pointer& p)
{
    uint16_t ret;
    const uint8_t a = Mem::readIncSafe(p);
    if(!(a & 0x80))
        ret = a;
    else
    {
        const uint8_t b = Mem::readIncSafe(p);
        ret = fglcd::make16(b, a & 0x7f);
    }
    return ret;
}



} // end namespace decomp
