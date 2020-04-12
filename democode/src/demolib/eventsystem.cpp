// The event system uses Timer 1 + interrupt
// The scheduler runs at millisecond granularity

#include "eventsystem.h"
#include "cfg-demo.h"
#include "globals.h"
#include "../demomath/fgmath.h"

#if CFG_MAX_EVENTS+0

#ifdef __AVR__

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "demo_def.h"

typedef evs::priv_Event Event;

static const long clocksPerMillisec = F_CPU / 1000UL;
static const unsigned MAX_TIMER_VALUE = 0xffff;

#if CFG_USE_HIGH_PRECISION_EVENT_TIMER+0
#define enableTimer() \
    TCCR1B = _BV(CS11) | _BV(CS10) | (1 << WGM12); // start clock with /64 prescaler
static const long PRESCALER = 64;
#else
#define enableTimer() \
    TCCR1B = _BV(CS12) | _BV(CS10) | (1 << WGM12); // start clock with /1024 prescaler + CTC mode
static const long PRESCALER = 1024;
#endif


#define disableTimer() \
    TCCR1B = (1 << WGM12);

#define enableTimerInterrupt() \
    TIMSK1 = (1 << OCIE1A); // enable interrupt on match (== OCR2A)

#define disableTimerInterrupt() \
    TIMSK1 = 0;

static const long ticksPerMillisec = clocksPerMillisec / PRESCALER;
#define msToTicks(ms) ( long(ms) * ticksPerMillisec )
#define ticksToMS(ticks) ( (ticks) / ticksPerMillisec )

void evs::init()
{
    G.ev.inISR = 0;
    G.ev.numEv = 0;
    G.ev.numNewEvents = 0;
    G.ev.waittime = MAX_TIMER_VALUE;

    disableTimer();
    TCCR1A = 0;
    TCNT1 = 0;
    OCR1A = MAX_TIMER_VALUE;
    enableTimerInterrupt();
}

static void updateTimer(unsigned ms)
{
    const unsigned ticks = (unsigned)vmin<long>(msToTicks(ms), MAX_TIMER_VALUE);
    G.ev.waittime = ticksToMS(ticks);
    TCNT1 = 0;
    OCR1A = ticks;
}

void evs::schedule(unsigned ms, callback f, void * ud)
{
    FGLCD_ASSERT(ms, "esched0"); // makes problems somehow
    if(!G.ev.inISR)
    {
        const uint8_t oldTIMSK1 = TIMSK1;
        disableTimerInterrupt();
        __builtin_avr_nop(); // allow any pending interrupt to trigger
        const uint8_t n = G.ev.numEv;
        volatile Event *ev = &G.ev.events[n]; // write to front
        ev->remain = ms;
        ev->f = f;
        ev->ud = ud;
        G.ev.numEv = n + 1;
        if(ms <= G.ev.waittime) // <= is important, if this is the first task waittime == 0xffff, and if ms is also == 0xffff, this would not properly set the timer
            updateTimer(ms);
        enableTimer();
        TIMSK1 = oldTIMSK1; // re-enable timer interrupt if previously enabled. may fire interrupt right away if ms == 0
    }
    else
    {
        G.ev.waittime = vmin(G.ev.waittime, ms);
        const uint8_t nn = G.ev.numNewEvents;
        G.ev.numNewEvents = nn + 1;
        volatile Event *ev = &G.ev.events[Countof(G.ev.events)-1 - nn]; // write to back
        ev->remain = ms;
        ev->f = f;
        ev->ud = ud;
    }
}

unsigned char evs::getNumEvents()
{
    return G.ev.numEv;
}

void evs::killAllEvents()
{
    G.ev.numEv = 0;
}

static void isr_updateEvents(uint8_t N)
{
    // N = Number of currently active events (not including those that got added inside of this very interrupt invocation)
    // numEv is not touched until the very end of the ISR.
    disableTimerInterrupt(); // leave other interrupts untouched -- do NOT disable globally since there might be more important ISRs to run
    unsigned wt = MAX_TIMER_VALUE; // allow pending interrupts to trigger
    COMPILER_BARRIER();
    ++G.ev.inISR;
    const unsigned timepassed = G.ev.waittime;
    for(uint8_t i = 0; i < N; )
    {
        volatile Event *ev = &G.ev.events[i];
        unsigned remain = ev->remain;
        if(remain > timepassed)
        {
            // Event is not yet due; update time and move to next
            remain -= timepassed;
            wt = vmin(wt, remain);
            ev->remain = remain;
            ++i;
            continue;
        }
        // --- Time to run the callback ---
        // Get data and store in registers
        evs::callback const f = ev->f;
        void * const ud = ev->ud;
        // One less now
        --N;
        // Move top event over current
        volatile Event *top = &G.ev.events[N];
        ev->remain = top->remain;
        ev->f = top->f;
        ev->ud = top->ud;

        G.ev.waittime = wt; // f() call may spawn new events, which depends on waittime set correctly
        f(ud);
        wt = G.ev.waittime; // re-fetch, as this may have changed
    }
    const uint8_t newEv = G.ev.numNewEvents;
    if(newEv)
    {
        memmove((void*)&G.ev.events[N], (const void*)&G.ev.events[Countof(G.ev.events) - newEv], newEv * sizeof(Event));
        N += newEv;
        G.ev.numNewEvents = 0;
    }
    G.ev.numEv = N;
    if(N)
    {
        updateTimer(wt);
    }
    else
    {
        disableTimer();
        G.ev.waittime = unsigned(-1);
    }
    --G.ev.inISR;
    enableTimerInterrupt();
}

ISR(TIMER1_COMPA_vect, ISR_NOBLOCK)
{
    uint8_t N = G.ev.numEv;
    if(N)
        isr_updateEvents(N);
}

#elif defined(MCU_IS_PC) // end __AVR__

static SDL_atomic_t numev = { 0 };

struct PassCB
{
    evs::callback f;
    void *ud;
};

static Uint32 _sdl_timer_thunk(Uint32 interval, void *pass)
{
    PassCB *p = (PassCB*)pass;
    SDL_AtomicAdd(&numev, -1);
    SDL_MemoryBarrierAcquire(); // yolo
    p->f(p->ud);
    SDL_MemoryBarrierRelease();
    delete p;
    return 0;
}

void evs::init() { SDL_AtomicSet(&numev, 0); }
void evs::schedule(unsigned ms, callback f, void *ud)
{
    FGLCD_ASSERT(ms, "esched0");
    unsigned evnow = SDL_AtomicAdd(&numev, 1) + 1;
    FGLCD_ASSERT(evnow < CFG_MAX_EVENTS, "evs overload");
    SDL_AddTimer(ms, _sdl_timer_thunk, new PassCB { f, ud });
}
unsigned char evs::getNumEvents()
{
    return SDL_AtomicGet(&numev);
}
void evs::killAllEvents()
{
    SDL_AtomicSet(&numev, 0);
}

#endif // MCU_IS_PC

#else

namespace evs {
unsigned char getNumEvents() { return 0; }
void init() {}
void schedule(unsigned ms, callback f, void *ud) {}
}

#endif
