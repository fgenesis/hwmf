#pragma once

#include <stdint.h>
#include "cfg-demo.h"
#include "fglcd/def.h"
#include "fglcd/macros.h"

#define PROGMEM_FAR FGLCD_PROGMEM_FAR

#if defined(_MSC_VER) || defined(unix)
#define LIKELY(x)      (x)
#define UNLIKELY(x)    (x)
#endif


#ifdef __AVR__

#pragma GCC optimize ("O3")

#include <avr/pgmspace.h>

#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wunused-function"
//#pragma GCC diagnostic error "-fpermissive"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"


#define COMPILER_BARRIER() __asm__ __volatile__ ("" : : : "memory")

// placement new decl, since #include <new> doesn't exist on AVR-libc
inline void* operator new(size_t _Size, void* _Where)
{
    (void)_Size;
    return _Where;
}

inline void operator delete(void*, void*)
{
    return;
}

template <size_t Size, size_t Align>
struct AlignedMemory {
    char mem[Size];
} __attribute__((aligned(Align)));


// This is quite a hack. Since PROGMEM starts filling the lower 64k ROM area and eventually goes beyond that,
// we need a way to start filling the high area.
// The default linker script has no such section defined, but .fini0 seems to work for the purpose
// as it comes after the code section (by default it does PROGMEM, code, destructors, finiX, in that order.
// Since we don't leave main(), it's never executed.
// Same thing is in fardata.S.
#ifndef PROGMEM_FAR
#define PROGMEM_FAR __attribute__((__section__(".fini0")))
#endif

#ifndef PFSTR
#define PFSTR(s) (__extension__({static const char __c[] PROGMEM_FAR = (s); &__c[0];}))
#endif

#define LIKELY(x)      __builtin_expect(!!(x), 1)
#define UNLIKELY(x)    __builtin_expect(!!(x), 0)

#endif

#ifndef Countof
namespace internal
{
    template <typename T, size_t N>
    char (&_ArraySizeHelper( T (&a)[N]))[N];

    template <typename A>
    char (&_ArraySizeHelper(const A&))[A::N + (0&typename A::is_array_tag())];

    template<size_t n>
    struct NotZero { static const size_t value = n; };
    template<>
    struct NotZero<0> {};

    constexpr size_t _StrlenHelper(const char * const s) { return *s ? 1 + _StrlenHelper(s + 1) : 0; }

}
#define Countof(a) (internal::NotZero<(sizeof(internal::_ArraySizeHelper(a)))>::value)
#define static_strlen(a) (internal::_StrlenHelper(a))
//#define static_strlen(a) (Countof(a)-1)
#endif

static FORCEINLINE uint8_t s8abs(int8_t x)
{
    const uint8_t mask = uint8_t(x) >> 7u;
    return (mask+x)^mask;
}
static FORCEINLINE uint16_t s16abs(int16_t x)
{
    const uint16_t mask = uint16_t(x) >> 15u;
    return (mask+x)^mask;
}

template<unsigned SH, typename T>
FORCEINLINE T rotl(T v)
{
    const T BITS = sizeof(T) * 8;
    const T s =  SH>=0? SH%32 : -((-SH)%BITS);
    return (v<<s) | (v>>(BITS-s));
}

template<unsigned SH, typename T>
FORCEINLINE T rotr(T v)
{
    const T BITS = sizeof(T) * 8;
    const T s =  SH>=0? SH%BITS : -((-SH)%BITS);
    return (v>>s) | (v<<(BITS-s));
}

FORCEINLINE static uint8_t ulo8(uint16_t w)
{
    union layout
    {
        uint16_t i;
        struct { uint8_t lo, hi; } b;
    } u;
    u.i = w;
    return u.b.lo;
}

FORCEINLINE static uint8_t uhi8(uint16_t w)
{
    union layout
    {
        uint16_t i;
        struct { uint8_t lo, hi; } b;
    } u;
    u.i = w;
    return u.b.hi;
}

FORCEINLINE static uint8_t uhh8(uint32_t w)
{
    union layout
    {
        uint32_t i;
        struct { uint8_t lo, hi, hhi, hhhi; } b;
    } u;
    u.i = w;
    return u.b.hhi;
}

FORCEINLINE static uint8_t uhhh8(uint32_t w)
{
    union layout
    {
        uint32_t i;
        struct { uint8_t lo, hi, hhi, hhhi; } b;
    } u;
    u.i = w;
    return u.b.hhhi;
}


/*
template<uint32_t x> struct TFac { static const uint32_t value = x * TFac<x-1>::value; };
template<> struct TFac<0u> { static const uint32_t value = 1u; };
template<uint32_t x, uint32_t p> struct TPow { static const uint32_t value = x * TFac<p-1>::value; };
template<uint32_t x> struct TPow<x, 0u> { static const uint32_t value = 1u; };
template<uint32_t v, uint32_t i, int m> struct TSinPart { static const float value = m * float(TPow<v, i>::value) / float(TFac<i>::value); };
template<uint32_t v, uint32_t i, int m> struct _TSinSumF { static const float value = TSinPart<v, i, m>::value + _TSinSumF<v, i+1u, -m>::value; };
template<uint32_t v, int m> struct _TSinSumF<v, 25, m> { static const float value = 0; };
template<uint32_t v> struct TSinSumF { static const float value = _TSinSumF<v, 1u, 1>::value; };

template<unsigned A>
static FORCEINLINE void *alignPtr(void *p)
{
    return (uint8_t*)((uint_farptr_t)p + (A - (uint_farptr_t)p % A));
}
*/

template<unsigned A>
static FORCEINLINE unsigned alignedSize(unsigned sz)
{
    return sz + A;
}

#define fastmemcpy_PF(dst, src, n) fglcd::ProgmemFar::Memcpy(dst, fglcd_get_farptr(src), n)

template <typename T> struct remove_ref            { typedef T type; };
template <typename T> struct remove_ref<T&>        { typedef T type; };
template <typename T> struct remove_ref<T&&>       { typedef T type; };

template <typename T> struct remove_pointer            { typedef T type; };
template <typename T> struct remove_pointer<T*>        { typedef typename remove_pointer<T>::type type; };

template <typename T> struct remove_array { typedef T type; };
template <typename T> struct remove_array<T[]> { typedef typename remove_array<T>::type type; };
template <typename T, size_t N> struct remove_array<T[N]> { typedef typename remove_array<T>::type type; enum { size = N }; };

template <typename T> struct remove_const                { typedef T  type; };
template <typename T> struct remove_const<const T&>      { typedef T& type; };
template <typename T> struct remove_const<const T>      { typedef T type; };

template <typename T> struct remove_volatile             { typedef T  type; };
template <typename T> struct remove_volatile<volatile T> { typedef T type; };

template <typename T> struct remove_cv;
template <typename T> struct remove_cv
{
    typedef
        typename remove_volatile <
            typename remove_const<T>::type
        >::type
        type;
};

/*
template<typename TA>
struct PFArray
{
    typedef typename remove_array<
        typename remove_cv<
            typename remove_ref<TA>::type
        >::type
    >::type T;

    enum { N = remove_array<TA>::size };

    T arr[N];

    PFArray(fglcd::FarPtr pf)
    {
        fglcd::ProgmemFar::Memcpy(arr, pf, N * sizeof(T));
    }

    FORCEINLINE T& operator[](size_t i) { return arr[i]; }
    FORCEINLINE const T& operator[](size_t i) const { return arr[i]; }

    FORCEINLINE operator T*() { return &arr[0]; }
    FORCEINLINE operator const T*() const { return &arr[0]; }

    FORCEINLINE operator void*() { return &arr[0]; }
    FORCEINLINE operator const void*() const { return &arr[0]; }

    //FORCEINLINE operator fglcd::RAM::Pointer() const { return static_cast<fglcd::RAM::Pointer>(&arr[0]); }
};

#define FARARRAY(var, af) PFArray<decltype(af)> var(fglcd_get_farptr(af))
*/

#define demopart NOINLINE void
