#pragma once

// only intended for storage, NOT for everyday math!

#include "_impl_master.h"

#include "../demolib/demo_def.h"

struct fp88
{
    typedef uint8_t u8;
    typedef int8_t s8;
    typedef uint16_t u16;
    typedef uint32_t u32;
    typedef int16_t s16;
    typedef int32_t s32;

    static const constexpr float MANTISSA_DIV = float(INTMAX_C(0xff)+INTMAX_C(1));

    // Iiii iiii iiii mmmm, two's complement
    union layout
    {
        s16 i;
        struct { u8 lo; s8 hi; } part;

        FORCEINLINE layout() {}
        FORCEINLINE constexpr layout(s8 realpart, u8 mantissa)
            : i((u16(realpart) << u16(8)) | mantissa)
        {}
        FORCEINLINE constexpr layout(float in)
            : i((in < 0) ? -_fromfloat(-in) : _fromfloat(in))
        {}
        FORCEINLINE constexpr layout(u16 raw)
            : i(raw)
        {}

     private:
        static constexpr s16 _fromfloat(float a)
        {
            return _mantissa(a) | (u8(int(a)) << s16(8));
        }
        static constexpr u8 _mantissa(float a)
        {
            return u8((a - float(int(a))) * MANTISSA_DIV);
        }
    } f;

    struct _raw_tag {};

    FORCEINLINE fp88() {}
    FORCEINLINE constexpr fp88(u16 x, _raw_tag) : f(x) {}
    FORCEINLINE fp88(const fp88& in) : f(in.f) {}
    FORCEINLINE fp88(const fp88&& in) : f(in.f) {}
    FORCEINLINE constexpr fp88(const layout& in) : f(in) {}
    FORCEINLINE constexpr fp88(s8 in) : f(in, 0) {}
    FORCEINLINE constexpr fp88(s8 in, u8 mantissa) : f(in, mantissa) {}
    FORCEINLINE constexpr fp88(float in) : f(in) {}
    FORCEINLINE constexpr fp88(int in) : f(in, 0) {}
    static FORCEINLINE constexpr fp88 raw(u16 in) { return fp88(in, _raw_tag()); }
    FORCEINLINE fp88& operator=(const fp88& o) { f.i = o.f.i; return *this; }
    template<typename T>
    FORCEINLINE fp88 operator*(const T& o) const { fp88 t = *this; t.mul(o); return t; }
    template<typename T>
    FORCEINLINE fp88& operator*=(const T& o) { mul(o); return *this;}
    FORCEINLINE fp88 operator+(const fp88& o) const { fp88 r = f; r.f.i += o.f.i; return r; }
    FORCEINLINE fp88& operator+=(const fp88& o) { f.i += o.f.i; return *this;}
    FORCEINLINE fp88 operator-(const fp88& o) const { fp88 r = f; r.f.i -= o.f.i; return r; }
    FORCEINLINE fp88& operator-=(const fp88& o) { f.i -= o.f.i; return *this;}
    FORCEINLINE fp88 operator-() const { fp88 o; o.f.i = -f.i; return o; }
    FORCEINLINE fp88 operator+() const { return *this; }
    template<typename T>
    FORCEINLINE fp88 operator/(const T& o) const { fp88 t = *this; t.div(o); return t; }
    template<typename T>
    FORCEINLINE fp88& operator/=(const T& o) { div(o); return *this;}

    FORCEINLINE bool operator==(const fp88& o) const { return f.i == o.f.i; }
    FORCEINLINE bool operator!=(const fp88& o) const { return f.i != o.f.i; }
    FORCEINLINE bool operator< (const fp88& o) const { return f.i <  o.f.i; }
    FORCEINLINE bool operator<=(const fp88& o) const { return f.i <= o.f.i; }
    FORCEINLINE bool operator> (const fp88& o) const { return f.i >  o.f.i; }
    FORCEINLINE bool operator>=(const fp88& o) const { return f.i >= o.f.i; }

    FORCEINLINE u8 mantissa() const { return f.part.lo; }
    FORCEINLINE s8 intpart() const { return f.part.hi; }
    //FORCEINLINE operator s8() const { return intpart(); }
    //FORCEINLINE operator float() const { return tofloat(); }
    FORCEINLINE float tofloat() const { return float(f.i) / MANTISSA_DIV; }
    FORCEINLINE uint16_t raw() const { return f.i; }

    void mul(const fp88& o)
    {
        f.i = fp88_mul(f.i, o.f.i);
    }

    template<typename T>
    FORCEINLINE void mul(const T& o)
    {
        f.i *= o;
    }
    void div(const fp88& o)
    {
        f.i = fp88_div(f.i, o.f.i);
    }
    template<typename T>
    FORCEINLINE void div(const T& o)
    {
        f.i /= o;
    }
};
