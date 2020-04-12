#define NO_SDL

#include "lookup.h"
#include "../demolib/demo_def.h"
#include "../demolib/fglcd/fglcd.h"

static fp88 readtanval(fglcd::FarPtr p, uint8_t i)
{
    uint8_t idx = i & 0x7f;
    uint16_t rawval = fglcd::ProgmemFar::template read<uint16_t>(p, idx);
    return fp88::raw(rawval) * fp88::raw(0x91);
}


fp88 tan88slow(uint8_t x)
{
    fglcd::FarPtr tab = fglcd_get_farptr(lut_tantab);
    return readtanval(tab, x);
}

fp88 invtan88slow(uint8_t x)
{
    fglcd::FarPtr tab = fglcd_get_farptr(lut_invtantab);
    return readtanval(tab, x);
}
