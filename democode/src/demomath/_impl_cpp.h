#pragma once

inline static constexpr uint8_t avgfast_CPP(uint8_t a, uint8_t b)
{
    return (uint16_t(a)+b)/2u;
}

inline static constexpr uint8_t avgfast4_CPP(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return (uint16_t(a)+b+c+d)/4u;
}

// unsigned
inline static constexpr uint8_t scale8_CPP(uint8_t i, uint8_t scale)
{
    return (((uint16_t)i) * (1+(uint16_t)(scale))) >> 8;
}

// signed
inline static constexpr uint8_t scale8_CPP(int8_t i, uint8_t scale)
{
    return (((int16_t)i) * (1+(int16_t)(scale))) >> 8;
}

// unsigned
inline static constexpr uint16_t scale16by8_CPP(uint16_t i, uint8_t scale)
{
    return scale
        ? (i * (1+((uint16_t)scale))) >> 8
        : i;
}

// signed
inline static constexpr int16_t scale16by8_CPP(int16_t i, uint8_t scale)
{
    // FIXME: THIS IS BROKEN ON AVR??!
    return scale
        ? (i * (1+((int16_t)scale))) >> 8
        : i;
}

inline static constexpr uint16_t scale16_CPP(uint16_t i, uint16_t scale)
{
    return ((uint32_t)(i) * (1+(uint32_t)(scale))) / UINT32_C(65536);
}

inline static constexpr uint8_t saturateAdd_CPP(uint8_t a, uint8_t b)
{
    return (uint8_t(a+b) < a) /* Can only happen due to overflow */
        ? uint8_t(0xff)
        : uint8_t(a+b);
}

inline static constexpr uint8_t saturateSub_CPP(uint8_t a, uint8_t b)
{
    return (uint8_t(a-b) > a) /* Can only happen due to overflow */
        ? uint8_t(0)
        : uint8_t(a-b);
}


inline static constexpr int32_t fp1616_mul_CPP(int32_t a, int32_t b)
{
    return int32_t((int64_t(a) * int64_t(b)) >> int64_t(16));
}

inline static constexpr int16_t fp1616_intdiv_CPP(int32_t a, int32_t b)
{
    return int16_t(a / b);
}

inline static constexpr int32_t fp1616_div_CPP(int32_t a, int32_t b)
{
    return int32_t((int64_t(a) << int64_t(16)) / int64_t(b));
}



inline static constexpr int16_t fp88_mul_CPP(int16_t a, int16_t b)
{
    return int16_t((int32_t(a) * int32_t(b)) >> int32_t(8));
}

inline static constexpr int8_t fp88_intdiv_CPP(int32_t a, int32_t b)
{
    return int8_t(a / b);
}

inline static constexpr int16_t fp88_div_CPP(int16_t a, int16_t b)
{
    return int16_t((int32_t(a) << int32_t(8)) / int32_t(b));
}
