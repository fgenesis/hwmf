#include <stdint.h>
#include "../demolib/demo_def.h"
#include "_impl_cpp.h"


FORCEINLINE void _adc(uint8_t& carry, uint8_t& dst, uint8_t x)
{
    uint16_t res = uint16_t(dst) + uint16_t(carry) + uint16_t(x);
    dst = uint8_t(res);
    carry = res >> 8;
}

FORCEINLINE void _addmul(uint8_t& carry, uint8_t& dstlo, uint8_t& dsthi, uint8_t a, uint8_t b)
{
    uint16_t res = uint16_t(a) * b;
    uint8_t lo = uint8_t(res);
    uint8_t hi = uint8_t(res >> 8);
    carry = 0;
    _adc(carry, dstlo, lo);
    _adc(carry, dsthi, hi);
}

int32_t fp1616_mul_CPP_test(int32_t a_, int32_t b_)
{
    uint8_t neg = 0;
    if(a_ < 0)
    {
        a_ = -a_;
        ++neg;
    }
    if(b_ < 0)
    {
        b_ = -b_;
        ++neg;
    }

    uint32_t a = a_;
    uint32_t b = b_;

    uint8_t A = uint8_t(a);
    uint8_t B = uint8_t(a >> 8);
    uint8_t C = uint8_t(a >> 16);
    uint8_t D = uint8_t(a >> 24);

    uint8_t X = uint8_t(b);
    uint8_t Y = uint8_t(b >> 8);
    uint8_t Z = uint8_t(b >> 16);
    uint8_t W = uint8_t(b >> 24);

    uint8_t r0=0, r1=0, r2=0, r3=0, r4=0, r5=0, r6=0, r7=0, carry=0;

    if(A)
    {
        _addmul(carry, r0, r1, A, X);
        _addmul(carry, r1, r2, A, Y);
        _addmul(carry, r2, r3, A, Z);
        _addmul(carry, r3, r4, A, W);
    }
    if(B)
    {
        _addmul(carry, r1, r2, B, X);
        _addmul(carry, r2, r3, B, Y);
        _addmul(carry, r3, r4, B, Z);
        _addmul(carry, r4, r5, B, W);
    }
    if(C)
    {
        _addmul(carry, r2, r3, C, X);
        _addmul(carry, r3, r4, C, Y);
        _addmul(carry, r4, r5, C, Z);
        _addmul(carry, r5, r6, C, W);
    }
    if(D)
    {
        _addmul(carry, r3, r4, D, X);
        _addmul(carry, r4, r5, D, Y);
        _addmul(carry, r5, r6, D, Z);
        //_addmul(carry, r6, r7, D, W);
    }

    int32_t res = (uint32_t(r2))
         | (uint32_t(r3) << 8)
         | (uint32_t(r4) << 16)
         | (uint32_t(r5) << 24);

    if(neg == 1)
        res = -res;

    return res;
}

