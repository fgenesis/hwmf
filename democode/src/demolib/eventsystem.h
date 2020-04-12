#pragma once

#include "cfg-demo.h"

namespace evs {

typedef void (*callback)(void *);

struct priv_Event
{
    unsigned remain;
    evs::callback f;
    void *ud;
};

struct priv_Globals
{
    volatile uint8_t numEv, numNewEvents;
    volatile uint8_t inISR;
    volatile priv_Event events[CFG_MAX_EVENTS];
    volatile unsigned waittime;
};


void init();
void schedule(unsigned ms, callback f, void *ud);
unsigned char getNumEvents();
void killAllEvents(); // NEVER call this from an event function
}
