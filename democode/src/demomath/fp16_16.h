#pragma once

#include "fp8_8.h"
#include "_impl_master.h"

struct fp1616
{
    typedef uint8_t u8;
    typedef int8_t s8;
    typedef uint16_t u16;
    typedef uint32_t u32;
    typedef int16_t s16;
    typedef int32_t s32;

    struct _raw_tag {};

    // These UINTMAX_C shenanigans are because avr-gcc flattens 0xffff+1 to 0 ??
    // Also see https://godbolt.org/z/ZVYuTY
    static const constexpr float MANTISSA_DIV = float(INTMAX_C(0xffff)+INTMAX_C(1));

    // Iiii iiii iiii iiii mmmm mmmm mmmm mmmm, two's complement
    union layout
    {
        s32 i;
        struct { u16 lo; s16 hi; } part;

        FORCEINLINE layout() {}
        FORCEINLINE constexpr layout(s16 realpart, u16 mantissa)
            : i((u32(realpart) << u32(16)) | mantissa)
        {}
        FORCEINLINE constexpr layout(float in)
            : i((in < 0) ? -_fromfloat(-in) : _fromfloat(in))
        {}
        FORCEINLINE constexpr layout(u32 raw, _raw_tag)
            : i(raw)
        {}

    private:
        static constexpr s32 _fromfloat(float a)
        {
            return _mantissa(a) | (u32(int(a)) << u32(16));
        }
        static constexpr u16 _mantissa(float a)
        {
            return u16((a - float(int(a))) * MANTISSA_DIV);
        }
    } f;

    FORCEINLINE fp1616() {}
    FORCEINLINE constexpr fp1616(u32 x, _raw_tag) : f(x, _raw_tag()) {}
    FORCEINLINE fp1616(const fp1616& in) : f(in.f.i, _raw_tag()) {}
    FORCEINLINE fp1616(const fp1616&& in) : f(in.f.i, _raw_tag()) {}
    FORCEINLINE constexpr fp1616(const layout& in) : f(in) {}
    //FORCEINLINE fp1616(s16 in) : f(in, 0) {}
    FORCEINLINE constexpr fp1616(float in) : f(in) {}
    FORCEINLINE constexpr fp1616(int in) : f(in, 0) {}
    FORCEINLINE constexpr fp1616(unsigned in) : f((int)in, 0) {}
    FORCEINLINE fp1616(const fp88& in) : f(s16(in.intpart()), u16(in.mantissa()) << 8u) {}
    static FORCEINLINE constexpr fp1616 raw(u32 in) { return fp1616(in, _raw_tag()); }
    static FORCEINLINE constexpr fp1616 raw(s16 i, u16 m) { return fp1616(layout(i, m)); }
    FORCEINLINE fp1616& operator=(const fp1616& o) { f.i = o.f.i; return *this; }
    template<typename T>
    FORCEINLINE fp1616 operator*(const T& o) const { fp1616 t = *this; t.mul(o); return t; }
    template<typename T>
    FORCEINLINE fp1616& operator*=(const T& o) { mul(o); return *this;}

    // kludge to allow compile-time constexpr multiplication so we don't have to touch op*.
    // gcc has __builtin_constant_p() but there's no MSVC equivalent and std::is_constant_evaluated() is still a long way off.
    static constexpr fp1616 cmul(const fp1616& a, const fp1616& b)
    {
        return raw(fp1616_mul_CPP(a.f.i, b.f.i));
    }

    FORCEINLINE constexpr fp1616 operator+(const fp1616& o) const { return raw(f.i + o.f.i); }
    FORCEINLINE fp1616& operator+=(const fp1616& o) { f.i += o.f.i; return *this;}
    FORCEINLINE constexpr fp1616 operator-(const fp1616& o) const { return raw(f.i - o.f.i); }
    FORCEINLINE fp1616& operator-=(const fp1616& o) { f.i -= o.f.i; return *this;}
    FORCEINLINE fp1616 operator-() const { return raw(-f.i); }
    FORCEINLINE fp1616 operator+() const { return *this; }
    template<typename T>
    FORCEINLINE fp1616 operator/(const T& o) const { fp1616 t = *this; t.div(o); return t; }
    template<typename T>
    FORCEINLINE fp1616& operator/=(const T& o) { div(o); return *this; }
    FORCEINLINE bool operator!() const { return !f.i; }

    FORCEINLINE bool operator==(const fp1616& o) const { return f.i == o.f.i; }
    FORCEINLINE bool operator!=(const fp1616& o) const { return f.i != o.f.i; }
    FORCEINLINE bool operator< (const fp1616& o) const { return f.i < o.f.i; }
    FORCEINLINE bool operator<=(const fp1616& o) const { return f.i <= o.f.i; }
    FORCEINLINE bool operator> (const fp1616& o) const { return f.i > o.f.i; }
    FORCEINLINE bool operator>=(const fp1616& o) const { return f.i >= o.f.i; }

    FORCEINLINE constexpr u32 raw() const { return f.i; }
    FORCEINLINE constexpr u16 mantissa() const { return f.part.lo; }
    FORCEINLINE constexpr s16 intpart() const { return f.part.hi; }

    FORCEINLINE float tofloat() const { return float(f.i) / MANTISSA_DIV; }

    FORCEINLINE fp88 tofp88() const { return fp88((s8)ulo8(intpart()), uhi8(mantissa())); }

    void mul(const fp1616& o)
    {
        f.i = fp1616_mul(f.i, o.f.i);
    }
    // FIXME: optimized mul with fp8.8
    void mul(const fp88& o)
    {
        mul(fp1616(o));
    }

    template<typename T>
    FORCEINLINE void mul(const T& o)
    {
        f.i *= o;
    }

    void div(const fp1616& o)
    {
        f.i = fp1616_div(f.i, o.f.i);
    }
    template<typename T>
    FORCEINLINE void div(const T& o)
    {
        f.i /= o;
    }

    FORCEINLINE int16_t int16div(const fp1616& by) const
    {
        return fp1616_intdiv(f.i, by.f.i);
    }
};


