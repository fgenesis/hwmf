#ifdef __AVR__

#include "_impl_avr.h"

// let the compiler figure out the signed cases

int8_t scale8_AVR(int8_t i, uint8_t scale)
{
    return scale8_CPP(i, scale);
}

int16_t scale16by8_AVR(int16_t i, uint8_t scale)
{
    return scale16by8_CPP(i, scale);
}


uint16_t scale16_AVR(uint16_t i, uint16_t scale)
{
    const uint8_t zero = 0;
    uint32_t result;
    asm volatile(
        "mul %A[i], %A[scale]     \n\t"
        "movw %A[result], r0      \n\t"
        "mul %B[i], %B[scale]     \n\t"
        "movw %C[result], r0      \n\t"
        "mul %B[i], %A[scale]     \n\t"
        "add %B[result], r0       \n\t"
        "adc %C[result], r1       \n\t"
        "adc %D[result], %[zero]  \n\t"
        "mul %A[i], %B[scale]     \n\t"
        "add %B[result], r0       \n\t"
        "adc %C[result], r1       \n\t"
        "adc %D[result], %[zero]  \n\t"
        "clr r1                   \n\t"
        "add %A[result], %A[i]    \n\t"
        "adc %B[result], %B[i]    \n\t"
        "adc %C[result], %[zero]  \n\t"
        "adc %D[result], %[zero]  \n\t"
        : [result] "=&r" (result)
        : [i] "r" (i), [scale] "r" (scale), [zero] "r" (zero)
    );
    return result >> 16;
}

/* Multiplication table:
32bit x 32bit is a 64 bit result, so we need 8 regs to store the result, in theory
Following the CPP implementation, we only want regs 5432
(last 2 are only for temporaries, first 2 are not needed)

DCBA x WZYX
The algorithm works via many 8bit multiplcations and summing those up.
// + indicates a carry must be added to the result (add carry to the reg where the + is)
// we can skip adding the carry where we know things won't overflow,
// and since we don't need the two upper regs marked with # at all, we can skip all operations on them)
registers:
76543210
##    ax (this means: multiply bytes A and X from the inputs, add each half to regs 0 and 1, and the carry to reg 2)
##   ay
##  az
## aw
##+++bx (from here, make sure to add the carry)
##++by
##+bz
##bw
##++cx
##+cy
##cz
#cw     (only need the lower byte result of the multiplcation)
##+dx
##dy
#dz     (only need the lower byte result of the multiplcation)
dw      (don't need this at all)
*/

// via http://www.vfx.hu/avr/download/mult32.asm and modified
FORCEINLINE static uint32_t fp1616_umul32x32(uint32_t x, uint32_t y)
{
    uint32_t ret;
    const uint8_t zero = 0;
    uint16_t tmp;
    asm volatile(
        "clr r0      \n\t"
        "movw %A[ret], r0      \n\t"
        "movw %C[ret], r0      \n\t"

        "mul	%A[x],%A[y]      \n\t"
        "movw	%A[tmp],R0       \n\t"

        "A_%=:       \n\t"
        "tst %A[y]  \n\t"
        "breq B_%=  \n\t"
        "mul	%B[x],%A[y]      \n\t"
        "add	%B[tmp],R0       \n\t"
        "adc	%A[ret],R1       \n\t"
        "mul	%C[x],%A[y]      \n\t"
        "add	%A[ret],R0       \n\t"
        "adc	%B[ret],R1       \n\t"
        "mul	%D[x],%A[y]      \n\t"
        "add	%B[ret],R0       \n\t"
        "adc	%C[ret],R1       \n\t"

        "B_%=:       \n\t"
        "tst %B[y]  \n\t"
        "breq C_%=  \n\t"
        "mul	%A[x],%B[y]      \n\t"
        "add	%B[tmp],R0       \n\t"
        "adc	%A[ret],R1       \n\t"
        "adc	%B[ret],%[zero]  \n\t"
        "adc	%C[ret],%[zero]  \n\t"
        "adc	%D[ret],%[zero]  \n\t"
        "mul	%B[x],%B[y]      \n\t"
        "add	%A[ret],R0       \n\t"
        "adc	%B[ret],R1       \n\t"
        "adc	%C[ret],%[zero]  \n\t"
        "adc	%D[ret],%[zero]  \n\t"
        "mul	%C[x],%B[y]      \n\t"
        "add	%B[ret],R0       \n\t"
        "adc	%C[ret],R1       \n\t"
        "adc	%D[ret],%[zero]  \n\t"
        "mul	%D[x],%B[y]      \n\t"
        "add	%C[ret],R0       \n\t"
        "adc	%D[ret],R1       \n\t"

        "C_%=:       \n\t"
        "tst %C[y]  \n\t"
        "breq D_%=  \n\t"
        "mul	%A[x],%C[y]      \n\t"
        "add	%A[ret],R0       \n\t"
        "adc	%B[ret],R1       \n\t"
        "adc	%C[ret],%[zero]  \n\t"
        "adc	%D[ret],%[zero]  \n\t"
        "mul	%B[x],%C[y]      \n\t"
        "add	%B[ret],R0       \n\t"
        "adc	%C[ret],R1       \n\t"
        "adc	%D[ret],%[zero]  \n\t"
        "mul	%C[x],%C[y]      \n\t"
        "add	%C[ret],R0       \n\t"
        "adc	%D[ret],R1       \n\t"
        "mul	%D[x],%C[y]      \n\t"
        "add	%D[ret],R0       \n\t"
        "adc	%G[ret],R1       \n\t"

        "D_%=:       \n\t"
        "tst %D[y]  \n\t"
        "breq E_%=  \n\t"
        "mul	%A[x],%D[y]      \n\t"
        "add	%B[ret],R0       \n\t"
        "adc	%C[ret],R1       \n\t"
        "adc	%D[ret],%[zero]  \n\t"
        "mul	%B[x],%D[y]      \n\t"
        "add	%C[ret],R0       \n\t"
        "adc	%D[ret],R1       \n\t"
        "mul	%C[x],%D[y]      \n\t"
        "add	%D[ret],R0       \n\t"

        "E_%=:       \n\t"
        "clr r1    \n\t"

        : [ret] "=&r" (ret), [tmp] "=&r" (tmp)
        : [x] "r" (x), [y] "r" (y), [zero] "r" (zero)
    );
    return ret;
}

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif
int32_t fp1616_mul_AVR(int32_t x, int32_t y)
{
    uint8_t neg = 0;
    if(x < 0)
    {
        x = -x;
        ++neg;
    }
    if(y < 0)
    {
        y = -y;
        ++neg;
    }

    int32_t ret = fp1616_umul32x32(x, y);

    if(neg == 1)
        ret = -ret;

    return ret;
}

int32_t fp1616_div_AVR(int32_t a, int32_t b)
{
    return fp1616_div_CPP(a, b); // TODO: this is very slow and should be replaced
}

#endif
