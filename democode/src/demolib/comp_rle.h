#pragma once

#include "decomp.h"


struct RLECompState
{
    RLECompState(void *dst);
    void addByte(uint8_t b);
    void addByteRep(uint8_t b, uint16_t n);
    unsigned flush(); // returns offset at which the run starts

    uint8_t *wptr, *ctrlp;
    unsigned offs;
    uint8_t lastbyte;
    uint8_t state;
    uint8_t ctr; // universal counter; different meaning per state
    uint8_t * const base;
};

// adapter for dynamic recompression template wizardry
struct DecompToRLE
{
    RLECompState state;

    DecompToRLE(void *dst) : state(dst) {}
    FORCEINLINE void emitSingleByte(uint8_t c) { state.addByte(c); }
    FORCEINLINE void emitMultiByte(uint8_t c, uint16_t len) { state.addByteRep(c, len); }
};

template<> struct SelectSideEffect<ToRLE> { typedef DecompToRLE type; };

