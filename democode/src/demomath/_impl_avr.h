#pragma once

#include "../demolib/demo_def.h"
#include "_impl_cpp.h"

FORCEINLINE static uint8_t avgfast_AVR(uint8_t a, uint8_t b)
{
    asm volatile(
        "add %[a], %[b]   \n\t"
        "ror %[a]"
        : [a] "+r" (a)
        : [b] "r" (b)
    );
    return a;
}

FORCEINLINE static uint8_t avgfast4_AVR(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    asm volatile(
        "add %[a], %[b]   \n\t"
        "ror %[a]   \n\t"
        "add %[c], %[d]   \n\t"
        "ror %[c]   \n\t"
        "add %[a], %[c]   \n\t"
        "ror %[a]"
        : [a] "+r" (a), [c] "+r" (c)
        : [b] "r" (b), [d] "r" (d)
    );
    return a;
}

FORCEINLINE static uint8_t scale8_AVR(uint8_t i, uint8_t scale)
{
    asm volatile(
        "mul %[a], %[s]       \n\t"
        "add r0, %[a]         \n\t" // throw value away but keep the carry
        "ldi %[a], 0x00       \n\t" // does NOT touch any flags (unlike clr)
        "adc %[a], r1         \n\t"
        "clr r1               \n\t"
        : [a] "+d" (i)      /* writes to i */
        : [s] "r"  (scale)  /* uses scale */
        : "r0", "r1"    /* clobbers r0, r1 */
    );
    return i;
}

FORCEINLINE static uint16_t scale16by8_AVR(uint16_t i, uint8_t scale)
{
    if(!scale)
        return i;
    uint16_t result = 0;
    asm volatile(
        "mul %A[i], %[scale]     \n\t"
        "add r0, %A[i]           \n\t"
        "adc %A[result], r1      \n\t"
        "mul %B[i], %[scale]     \n\t"
        "add %A[result], r0      \n\t"
        "adc %B[result], r1      \n\t"
        "clr r1                  \n\t"
        "add %A[result], %B[i]   \n\t"
        "adc %B[result], r1      \n\t"
        : [result] "+r" (result)
        : [i] "r" (i), [scale] "r" (scale)
        : "r0", "r1"
    );
    return result;
}

// signed is a bit trickier, let the compiler figure it out and don't inline it
int8_t scale8_AVR(int8_t i, uint8_t scale);
int16_t scale16by8_AVR(int16_t i, uint8_t scale);

// a bit too large to inline this one
uint16_t scale16_AVR(uint16_t i, uint16_t scale);


FORCEINLINE uint8_t saturateAdd_AVR(uint8_t a, uint8_t b)
{
    asm volatile(
        "add %[a], %[b]  \n\t"
        "brcc done_%=  \n\t"
        "ldi %[a], 0xff  \n\t"
        "done_%=:  \n\t"
        : [a] "+d" (a) /* outputs */
        : [b] "r" (b) /* inputs */
    );
    return a;
}

FORCEINLINE uint8_t saturateSub_AVR(uint8_t a, uint8_t b)
{
    asm volatile(
        "sub %[a], %[b]  \n\t"
        "brcc done_%=  \n\t"
        "clr %[a]  \n\t"
        "done_%=:  \n\t"
        : [a] "+r" (a) /* outputs */
        : [b] "r" (b) /* inputs */
    );
    return a;
}

// avg-gcc generates a redundant MOVW when MUL is used.
// Unsigned cases are easy (there are overloads for the nasty signed variants but those are not asm)
static FORCEINLINE void incrhiscale_AVR(uint8_t& a, uint8_t b, uint8_t c)
{
    asm volatile(
        "mul %[b],%[c]   \n\t"
        "add %[a], r1    \n\t"
        "clr r1"
        : [a] "+r" (a) /*outputs*/
        : [b] "r" (b), [c] "r" (c)
    );
}

static FORCEINLINE void incrhiscale_AVR(uint16_t& a, uint8_t b, uint8_t c)
{
    asm volatile(
        "mul %[b],%[c]   \n\t"
        "add %A[a], r1   \n\t"
        "clr r1          \n\t" // does not affect carry
        "adc %B[a], r1   \n\t"
        : [a] "+r" (a) /*outputs*/
        : [b] "r" (b), [c] "r" (c)
    );
}

static inline uint8_t hiscale8_AVR(uint8_t b, uint8_t c)
{
    asm volatile(
        "mul %[b],%[c]   \n\t"
        "mov %[b], r1    \n\t"
        "clr r1"
        : [b] "+r" (b)
        : [c] "r" (c)
    );
    return b;
}

FORCEINLINE static uint8_t avgfast16_AVR(uint8_t a, uint8_t b)
{
    asm volatile(
        "add %A[a], %A[b]    \n\t"
        "adc %B[a], %B[b]    \n\t"
        "ror %B[a]        \n\t"
        "ror %A[a]        \n\t"
        : [a] "+r" (a)
        : [b] "r"  (b)
    );
    return a;
}


int32_t fp1616_mul_AVR(int32_t a, int32_t b);
int32_t fp1616_div_AVR(int32_t a, int32_t b);

// this isn't so bad and probably the most efficient way to do this
FORCEINLINE int16_t fp1616_intdiv_AVR(int32_t a, int32_t b)
{
    return fp1616_intdiv_CPP(a, b);
}


// No asm impl for these for now.
// Those operations shouldn't happen anyway, as the precision is way too low.
FORCEINLINE int16_t fp88_mul_AVR(int16_t a, int16_t b)
{
    return fp88_mul_CPP(a, b);
}
FORCEINLINE int16_t fp88_div_AVR(int16_t a, int16_t b)
{
    return fp88_div_CPP(a, b);
}
FORCEINLINE int8_t fp88_intdiv_AVR(int16_t a, int16_t b)
{
    return fp88_intdiv_CPP(a, b);
}
