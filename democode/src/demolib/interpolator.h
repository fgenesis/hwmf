#pragma once

#include "../demomath/fgmath.h"

namespace interp {

typedef uint16_t Adj;

enum InterpolationStyle
{
    OFF,
    LERP_ONCE,
    LERP_SAWTOOTH
};

template<typename T>
struct InterpolatedValue
{
    T _val, start, end;

    FORCEINLINE InterpolatedValue() {}

    FORCEINLINE InterpolatedValue(T startval, T endval)
        : _val(startval), start(startval), end(endval)
    {}

    FORCEINLINE void update(Adj t)
    {
        _val = lerp(start, end, t);
    }

    FORCEINLINE operator T() const { return _val; }
};

struct _InterpolatedFancy
{
    volatile Adj _t;
    Adj _speed;
    volatile uint8_t _style; // InterpolationStyle
    void _finishTick();
};

template<typename T>
struct FancyInterpolatedValue : public InterpolatedValue<T>, public _InterpolatedFancy
{
    typedef FancyInterpolatedValue<T> Self;


    FancyInterpolatedValue(const T& v = T())
        : InterpolatedValue<T>(v, v)
    {
        this->_style = OFF;
        this->_t = 0;
    }

    ~FancyInterpolatedValue()
    {
        stop();
    }

    void interpolateTo(const T& newval, uint16_t speed, uint8_t style = LERP_ONCE)
    {
        fglcd::NoInterrupt no;
        this->_t = 0;
        this->start = this->_val;
        this->end = newval;
        this->_speed = speed;
        this->_style = style;
    }

    // TODO, IF NEEDED: interpolateTo() to trigger a user event after a delay

    FORCEINLINE void stop()
    {
        this->_style = OFF;
    }

    FORCEINLINE bool isInterpolating() const
    {
        return this->_style != OFF;
    }

    void tick()
    {
        if(this->_style == OFF)
            return;
        this->update(this->_t);
        this->_finishTick();
    }

    static NOINLINE void Tick(void *iv)
    {
        ((Self*)iv)->tick();
    }

    FORCEINLINE T& operator=(const T& v)
    {
        this->set(v);
        return *this;
    }

    void set(const T& v)
    {
        this->stop();
        this->_val = v;
    }

};

struct InterpolatorEntry
{
    void (*tickfunc)(void*);
    void *iv;
};

// Ticks a bunch of registered FancyInterpolatedValue
struct InterpolatorBase
{
    InterpolatorEntry * const _mem;
    uint16_t _tickrate;
    uint8_t _used;
    const uint8_t _cap;
    volatile uint8_t _taskstate; // 0:idle, 1:running, 2:exit request
    void registerTask();
    void unregisterTask();
    void waitTask();

    InterpolatorBase(uint16_t tickrate, InterpolatorEntry *mem, uint8_t n);

    static void TickCallback(void*);
    void _add(InterpolatorEntry& e);

public:

    ~InterpolatorBase();

    void start(); // for automatic, interrupt/event-driven ticking
    void start(uint16_t tickrate);
    void stop();
    FORCEINLINE void clear() { _used = 0; }

    void tick(); // for manual ticking

    // When adding a thing, that thing must outlive the interpolator!
    template<typename V>
    void add(V& iv)
    {
        InterpolatorEntry e;
        e.iv = &iv;
        e.tickfunc = V::Tick;
        _add(e);
    }
};

// Supposed to be allocated on the stack.
// Pass as N how many values to hold, max.
// Will assert that the buffer isn't overflowed
template<unsigned N>
struct Interpolator : public InterpolatorBase
{
    InterpolatorEntry _entries[N];

    FORCEINLINE Interpolator(uint16_t tickrate = 0)
        : InterpolatorBase(tickrate, &_entries[0], N)
    {}
};

} // end namespace interp
