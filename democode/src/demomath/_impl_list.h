#ifndef _IMPL
#error lolnope
#endif

_IMPL(uint8_t, avgfast, (uint8_t a, uint8_t b), (a,b) )
_IMPL(uint8_t, avgfast4, (uint8_t a, uint8_t b, uint8_t c, uint8_t d), (a,b,c,d))
_IMPL(uint8_t, scale8, (uint8_t a, uint8_t b), (a,b) )
_IMPL(int8_t, scale8, (int8_t a, uint8_t b), (a,b) )
_IMPL(uint16_t, scale16by8, (uint16_t a, uint8_t b), (a,b) )
_IMPL(int16_t, scale16by8, (int16_t a, uint8_t b), (a,b) )
_IMPL(uint16_t, scale16, (uint16_t a, uint16_t b), (a,b) )
_IMPL(uint8_t, saturateAdd, (uint8_t a, uint8_t b), (a,b) )
_IMPL(uint8_t, saturateSub, (uint8_t a, uint8_t b), (a,b) )

_IMPL(int32_t, fp1616_mul,    (int32_t a, int32_t b), (a,b) )
_IMPL(int32_t, fp1616_intdiv, (int32_t a, int32_t b), (a,b) )
_IMPL(int32_t, fp1616_div,    (int32_t a, int32_t b), (a,b) )

_IMPL(int16_t, fp88_mul,    (int16_t a, int16_t b), (a,b) )
_IMPL(int16_t, fp88_intdiv, (int16_t a, int16_t b), (a,b) )
_IMPL(int16_t, fp88_div,    (int16_t a, int16_t b), (a,b) )

#undef _IMPL
