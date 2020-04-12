#pragma once

#include "mathtypes.h"
#include "_impl_master.h"

template<typename T> static constexpr FORCEINLINE T vmax(T a, T b) { return a < b ? b : a; }
template<typename T> static constexpr FORCEINLINE T vmin(T a, T b) { return a < b ? a : b; }
template<typename T> static constexpr FORCEINLINE T vclamp(T v, T mi, T ma) { return vmax(vmin(v, ma), mi); }
template<typename T> static constexpr FORCEINLINE T vmin3(T a, T b, T c) { return vmin(a, vmin(b, c)); }
template<typename T> static constexpr FORCEINLINE T vmax3(T a, T b, T c) { return vmax(a, vmax(b, c)); }
template<typename T> static constexpr FORCEINLINE T vmin4(T a, T b, T c, T d) { return vmin(vmin(a, d), vmin(b, c)); }
template<typename T> static constexpr FORCEINLINE T vmax4(T a, T b, T c, T d) { return vmax(vmax(a, d), vmax(b, c)); }
template<typename T> static constexpr FORCEINLINE T vabs(T a) { return a >= T(0) ? a : -a; }
template<typename T> static constexpr FORCEINLINE T vsgn(T a) { return a ? (a >= T(0) ? 1 : -1) : 0; }

template<typename T> static FORCEINLINE void vswap(T& a, T& b) { T tmp = a; a = b; b = tmp; }
template<typename T> static FORCEINLINE void makeminmax(T& a, T& b) { if(b < a) vswap(a, b); }

template<typename T>
static FORCEINLINE void sort3(T& a, T& b, T& c)
{
    if(a > c)
        vswap(a, c);
    if(a > b)
        vswap(a, b);
    if(b > c)
        vswap(b, c);
}

template<typename T, T in>
struct NextPowerOf2
{
private:
    enum Tmp : uint64_t
    {
        v0 = in - 1,
        v1 = v0 | (v0 >> 1),
        v2 = v1 | (v1 >> 2),
        v3 = v2 | (v2 >> 4),
        v4 = v3 | (v3 >> 8),
        v5 = v4 | (v4 >> 16),
        v6 = v5 | (v5 >> 32),
        v7 = v6 + 1
    };
public:
    enum Result { value = T(v7) };
};
static_assert(NextPowerOf2<unsigned, 0>::value == 0, "pow2");
static_assert(NextPowerOf2<unsigned, 1>::value == 1, "pow2");
static_assert(NextPowerOf2<unsigned, 2>::value == 2, "pow2");
static_assert(NextPowerOf2<unsigned, 3>::value == 4, "pow2");
static_assert(NextPowerOf2<unsigned, 4>::value == 4, "pow2");
static_assert(NextPowerOf2<unsigned, 5>::value == 8, "pow2");
static_assert(NextPowerOf2<unsigned, 7>::value == 8, "pow2");

// wrap-around modulo without using operator%.
// value stays always positive or 0
template<typename T> static T vmodpos(T val, T mod)
{
    FGLCD_ASSERT(mod != 0, "mod 0");

    while(val >= mod)
        val -= mod;

    while(val < T(0))
        val += mod;

    return val;
}

template<typename T, T mod> static FORCEINLINE T vmodpos(T val)
{
    return vmodpos<T>(val, mod);
}

template<typename T>
struct _MoreBits {};

template<> struct _MoreBits<uint8_t> { typedef uint16_t type; FORCEINLINE static type convert(uint8_t v) { return v; }};
template<> struct _MoreBits<uint16_t> { typedef uint32_t type; FORCEINLINE static type convert(uint16_t v) { return v; }};
template<> struct _MoreBits<uint32_t> { typedef uint64_t type; FORCEINLINE static type convert(uint32_t v) { return v; }};
template<> struct _MoreBits<int8_t> { typedef int16_t type; FORCEINLINE static type convert(int8_t v) { return v; }};
template<> struct _MoreBits<int16_t> { typedef int32_t type; FORCEINLINE static type convert(int16_t v) { return v; }};
template<> struct _MoreBits<int32_t> { typedef int64_t type; FORCEINLINE static type convert(int32_t v) { return v; }};

template<typename T>
FORCEINLINE static typename _MoreBits<T>::type morebits(T x) { return _MoreBits<T>::convert(x); }

// BEWARE: T must be an unsigned type!
template<typename T>
FORCEINLINE T saturateAdd(T a, T b)
{
    static_assert(T(-1) > T(0), "signed types are UB here");
    T c = a + b;
    if (c < a) /* Can only happen due to overflow */
        c = T(-1);
    return c;
}

template<typename T>
FORCEINLINE T saturateSub(T a, T b)
{
    static_assert(T(-1) > T(0), "signed types are UB here");
    T c = a - b;
    if (c > a) /* Can only happen due to overflow */
        c = 0;
    return c;
}

template<typename B, typename C>
static FORCEINLINE B hiscale8(B b, C c)
{
    static_assert(sizeof(B) == 1, "expected 8 bit type");
    static_assert(sizeof(C) == 1, "expected 8 bit type");
    return (B)uhi8(b * morebits(c));
}

// Function useful for scaling values
template<typename T, typename B, typename C>
static FORCEINLINE void incrhiscale(T& a, B b, C c)
{
    a += hiscale8(b, c);
}


template<uintmax_t A, uintmax_t B>
struct GCD
{
    static constexpr uintmax_t largevalue = GCD<B, A % B>::value;
    typedef typename fglcd::TypeForSize<largevalue>::type value_type;
    static constexpr value_type value = largevalue;
};
template<uintmax_t A>
struct GCD<A, 0>
{
    static constexpr uintmax_t largevalue = A;
    typedef typename fglcd::TypeForSize<largevalue>::type value_type;
    static constexpr value_type value = largevalue;
};


template<uintmax_t A, uintmax_t B>
struct LCM
{
    static constexpr uintmax_t largevalue = (A*B) / GCD<A, B>::largevalue;
    typedef typename fglcd::TypeForSize<largevalue>::type value_type;
    static constexpr value_type value = largevalue;
};

/*
template <typename T>
tmat4_lookat<T> LookAt(const tvec3<T>& me, const tvec3<T>& dst, const tvec3<T>& up)
{
    const tvec3<T> f(normalize(dst - me));
    const tvec3<T> s(normalize(cross(f, up)));
    const tvec3<T> u(cross(s, f));
    
    tmat4_lookat<T> r;
#define S(x, y, v) r.template set<x, y>(v)
    S(0, 0, s.x);
    S(1, 0, s.y);
    S(2, 0, s.z);
    S(0, 1, u.x);
    S(1, 1, u.y);
    S(2, 1, u.z);
    S(0, 2, -f.x);
    S(1, 2, -f.y);
    S(2, 2, -f.z);
    S(3, 0, -dot(s, me));
    S(3, 1, -dot(u, me));
    S(3, 2,  dot(f, me));
#undef S
    return r;
};
*/

tmat4_persp<fp1616> tweakedInfinitePerspective(uint8_t fovy, fp1616 aspect);
tmat4_persp<fp1616> tweakedInfinitePerspective_InvAspect(uint8_t fovy, fp1616 iaspect);
tmat4_persp<fp1616> tweakedInfinitePerspective_NoAspect(uint8_t fovy);
tmat4_persp<fp1616> perspective(uint8_t fovy, fp1616 aspect);
tmat4_infperspLH<fp1616> infinitePerspectiveLH(uint8_t fovy, fp1616 aspect);


template<unsigned V, unsigned L>
struct Ilog2Trec
{
    enum { value = Ilog2Trec<(V >> 1u), L+1>::value };
};

template<unsigned L>
struct Ilog2Trec<0, L>
{
    enum { value = L };
};

template<unsigned V>
struct Ilog2T
{
    enum { value = Ilog2Trec<V, 0>::value - 1 };
};

fp1616 sqrt(fp1616 x);
fp1616 invsqrt(fp1616 x);

template<typename T, typename F>
T scaleT(T a, F t);

template<> FORCEINLINE
uint8_t scaleT<uint8_t, uint8_t>(uint8_t a, uint8_t t)
{
    return scale8(a, t);
}

template<> FORCEINLINE
uint16_t scaleT<uint16_t, uint8_t>(uint16_t a, uint8_t t)
{
    return scale16by8(a, t);
}
template<> FORCEINLINE
int16_t scaleT<int16_t, uint8_t>(int16_t a, uint8_t t)
{
    return scale16by8(a, t);
}

template<> FORCEINLINE
uint16_t scaleT<uint16_t, uint16_t>(uint16_t a, uint16_t t)
{
    return scale16(a, t);
}

// TODO: too lazy for an optimized 16x8 impl right now
template<> FORCEINLINE
uint8_t scaleT<uint8_t, uint16_t>(uint8_t a, uint16_t t)
{
    return (uint8_t)scale16(a, t);
}

/*
inline uint8_t brighten8(uint8_t x)
{
    uint8_t ix = 255 - x;
    return 255 - scale8( ix, ix);
}

inline uint8_t dim8(uint8_t x)
{
    return scale8(x, x);
}
*/

template<typename T, typename F>
FORCEINLINE static T lerp_inl(T a, T b, F frac)
{
    T result;
    if(b > a) {
        T delta = b - a;
        T scaled = scaleT(delta, frac);
        result = a + scaled;
    } else {
        T delta = a - b;
        T scaled = scaleT(delta, frac);
        result = a - scaled;
    }
    return result;
}

template<typename T, typename F>
NOINLINE static T lerp(T a, T b, F frac)
{
    return lerp_inl(a, b, frac);
}

template<typename V, typename F>
NOINLINE static V vlerp(const V& a, const V& b, F frac)
{
    V ret(noinit);
    for(vecitype i = 0; i < V::elems; ++i)
        ret[i] = lerp(a[i], b[i], frac);
    return ret;
}

template<typename T, typename F>
FORCEINLINE tvec2<T> lerp(const tvec2<T>& a, const tvec2<T>& b, F frac)
{
    return vlerp(a, b, frac);
}
template<typename T, typename F>
FORCEINLINE tvec3<T> lerp(const tvec3<T>& a, const tvec3<T>& b, F frac)
{
    return vlerp(a, b, frac);
}
template<typename T, typename F>
FORCEINLINE tvec4<T> lerp(const tvec4<T>& a, const tvec4<T>& b, F frac)
{
    return vlerp(a, b, frac);
}
