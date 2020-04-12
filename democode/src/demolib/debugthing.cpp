#include "debugthing.h"
#include "eventsystem.h"
#include "fglcd/chip_tm1638.h"
#include "../demomath/fgmath.h"
#include "cfg-demo.h"
#include "globals.h"
#include "demodebug.h"

#include <stdio.h>

uint8_t DebugThing::NumInstances()
{
#ifdef CFG_USE_DEBUG_THING
    return G.debug.numDebugInstances;
#endif
    return 0;
}

void DebugThing::Event(void *self)
{
    DebugThing *th = (DebugThing*)self;
    if(th->_done)
        th->_done = 2;
    else
    {
        const uint8_t b = Panel::getButtons();
        const uint8_t bx = b ^ th->_btn;
        if(th->_func)
            th->_func(*th, th->_ud, b, bx);
        th->_btn = b;
        th->schedule();
    }
}

void DebugThing::schedule()
{
    evs::schedule(50, Event, this);
}

void DebugThing::StaticInit()
{
    Panel::init();
    Panel::intensity(2);
}

DebugThing::DebugThing(Func f, void *ud)
    : DebugThing(f, ud, noinit)
{
#ifdef CFG_USE_DEBUG_THING
    G.debug.numDebugInstances++;
#endif
    init();
}

#ifdef CFG_USE_DEBUG_THING
DebugThing::DebugThing() // default init is used for the global debugger
    : DebugThing(dbg_demomain, &G.debug.vars, DebugThing::noinit)
{}

DebugThing::DebugThing(Func f, void *ud, NoInit)
 : _func(f), _ud(ud), _done(0)
{
}
#endif

void DebugThing::init()
{
    //Panel::clear();
    //Panel::setText<fglcd::Progmem>(PSTR("DEBUG"));
    _btn = Panel::getButtons(); // important to get the initial button configuration. (buttons are all 0xff if no panel is attached!)
    schedule();
}

DebugThing::~DebugThing()
{
    _done = 1;
    while(_done != 2) {}
    Panel::clear();
#ifdef CFG_USE_DEBUG_THING
    G.debug.numDebugInstances--;
#endif
}

void DebugThing::setNum(int n, uint8_t beg)
{
#ifdef CFG_USE_DEBUG_THING
    char buf[10];
    snprintf_P(&buf[0], sizeof(buf), PSTR("%8d "), n);
    setTextRAM(&buf[0], beg);
#endif
}

void DebugThing::set2Num(int x, int y)
{
#ifdef CFG_USE_DEBUG_THING
    x = vclamp(x, -999, 9999);
    y = vclamp(y, -999, 9999);
    char buf[9];
    snprintf_P(&buf[0], sizeof(buf), PSTR("%4d%4d"), x, y);
    setTextRAM(&buf[0]);
#endif
}

void DebugThing::set2NumHex(unsigned x, unsigned y)
{
#ifdef CFG_USE_DEBUG_THING
    char buf[9];
    snprintf_P(&buf[0], sizeof(buf), PSTR("%4X%4X"), x, y);
    setTextRAM(&buf[0]);
#endif
}

void DebugThing::setLEDs(uint8_t mask)
{
    Panel::setLEDs(mask);
}


void DebugThing::setTextRAM(const char * s, uint8_t beg)
{
    Panel::setText<fglcd::RAM>(s, beg);
}
void DebugThing::setRaw(uint8_t pos, uint8_t val)
{
    Panel::setRaw(pos, val);
}

void DebugThing::commit()
{
    Panel::commit();
}
