#pragma once

#include "demo_def.h"

class DebugThing;

struct DebugVars
{
    DebugVars();
    uint8_t what;
    uint8_t chHit[8];
};

void demo_init_debug();
void dbg_demomain(DebugThing& dbg, void *ud, uint8_t b, uint8_t bx);

#ifdef MCU_IS_PC


#include <stdio.h>
#include <iosfwd>
#include <iostream>

template<typename M>
static void printmat(const M& m)
{
    fglcd::NoInterrupt no;
    printf("--mat4[%d]--\n", M::dynamicsize);
    // fuck this shiiiit
#define G(x, y) (m.template get<x, y>().tofloat())
#define P(x, y) printf("%f ", G(x,y));
#define R(y) P(0,y) P(1,y) P(2,y) P(3,y) puts("");
    R(0) R(1) R(2) R(3)
#undef R
#undef G
#undef P
    puts("----");
}

template<typename M>
static void formatMatRawC(std::ostream& os, const M& m)
{
    for(unsigned i = 0 ; i < M::dynamicsize; ++i)
    {
        if(i && !(i%6))
            os << "\n  ";
        os << m._dyn[i].tofloat() << "f, ";
    }
}

template<typename M>
static void printmatRawC(const M& m)
{
    fglcd::NoInterrupt no;
    std::cout << "{";
    formatMatRawC(std::cout, m);
    std::cout << "}\n";
}

template<typename V>
static void printvec(const V& v)
{
    fglcd::NoInterrupt no;
    printf("-- vec%u (", V::elems);
    // fuck this shiiiit
#define G(x) (v[x].tofloat())
#define P(x) printf("%f ", G(x));
    for(unsigned i = 0; i < V::elems; ++i)
        P(i)
#undef G
#undef P
    puts(")");
}

#define DEBUG_PRINTF(...) printf(__VA_ARGS__)

#else

template<typename M>
static FORCEINLINE void printmat(const M& m) {}
template<typename V>
static FORCEINLINE void printvec(const V& v) {}

#define DEBUG_PRINTF(...) /* no-op */

#endif
