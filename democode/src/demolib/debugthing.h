#pragma once

#include "cfg-demo.h"
#include <stdio.h>

class DebugThing
{
public:
    enum NoInit { noinit };
    typedef void (*Func)(DebugThing&, void*, uint8_t buttons, uint8_t changed);

    void init();
    ~DebugThing();
    inline uint8_t getButtons() const { return _btn; }

    static void setTextRAM(const char *s, uint8_t beg = 0);
    static void setNum(int n, uint8_t beg = 0);
    static void set2Num(int x, int y);
    static void set2NumHex(unsigned x, unsigned y);
    static void setLEDs(uint8_t mask);
    static void setRaw(uint8_t pos, uint8_t val);
    static void commit();

    static FORCEINLINE uint8_t Alone() { return NumInstances() == 0; }

#ifndef CFG_USE_DEBUG_THING
private: // just disallow construction, is the easiest thing to prevent accidental instantiation
#endif
    DebugThing();
    DebugThing(Func, void*);
    DebugThing(Func, void*, NoInit);
    static void StaticInit();

private:
    static void Event(void *self);
    void schedule();
    Func const _func;
    void * const _ud;
    uint8_t _btn;
    volatile uint8_t _done;
    static uint8_t NumInstances(); // instances created with noinit flag are considered background and not counted
};

