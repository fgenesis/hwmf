#pragma once

// This is THE main debug switch.
// Comment out to kill ALL the asserts, checks, buttonpanel references, and whatnot.
#ifndef __AVR__
#define FGLCD_DEBUG
#endif

// And here goes ALL inline assembly
#define FGLCD_ASM


#ifdef _CHECK_PRAGMA_ONCE
#error pragma once disobeyed! wtf, gcc!
#else
#define _CHECK_PRAGMA_ONCE
#endif

#ifdef __GNUC__
#pragma GCC optimize ("O3")
#define FORCEINLINE __attribute__((always_inline)) inline
#define NOINLINE __attribute__((noinline))
#define ALIGN(x) __attribute__((aligned(x)))
#endif

#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#define NOINLINE __declspec(noinline)
#define ALIGN(x) __declspec(align(x))
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>



template<unsigned SZ, unsigned A>
struct ALIGN(A) AlignedMem
{
    uint8_t mem[SZ];
};

namespace fglcd {

namespace types {

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int32_t s64;

}

using namespace types;

struct NoInstance
{
private:
    NoInstance();
};

namespace priv {

template <typename T, T v>
struct IntegralConstant
{
    typedef T value_type;
    typedef IntegralConstant<T,v> type;
    enum { value = v };
};

typedef IntegralConstant<bool, true>  CompileTrue;
typedef IntegralConstant<bool, false> CompileFalse;
template<bool V> struct CompileCheck : IntegralConstant<bool, V>{};

} // end namespace priv

template <typename T, typename U> struct is_same      : priv::CompileFalse { };
template <typename T>             struct is_same<T,T> : priv::CompileTrue  { };

template<bool COND, typename A, typename B> struct TypeSwitch{};
template <typename A, typename B> struct TypeSwitch<true, A, B> { typedef A type; };
template <typename A, typename B> struct TypeSwitch<false, A, B> { typedef B type; };



} // end namespace fglcd
