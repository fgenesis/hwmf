#include "interpolator.h"
#include "eventsystem.h"
#include "fglcd/macros.h"

namespace interp {

void InterpolatorBase::TickCallback(void *p)
{
    InterpolatorBase *base = (InterpolatorBase*)p;
    if(base->_taskstate == 2)
    {
        base->_taskstate = 0;
        return;
    }
    base->tick();
    base->registerTask();
}


void InterpolatorBase::registerTask()
{
    evs::schedule(_tickrate, TickCallback, this);
}

void InterpolatorBase::unregisterTask()
{
    if(_taskstate == 1)
        _taskstate = 2;
}

void InterpolatorBase::waitTask()
{
    switch(_taskstate)
    {
        case 0:
            return;
        case 1:
            _taskstate = 2;
            // fall through
        case 2:
            while(_taskstate) {}
    }
}

void InterpolatorBase::stop()
{
    unregisterTask();
    // but don't wait
}

void InterpolatorBase::start()
{
    FGLCD_ASSERT(_taskstate != 1, "interptsk");
    // if task is running, wait until it's gone and
    waitTask(); // _taskstate is 0 after this
    registerTask();
    _taskstate = 1;
}

void InterpolatorBase::start(uint16_t tickrate)
{
    _tickrate = tickrate;
    start();
}

InterpolatorBase::InterpolatorBase(uint16_t tickrate, InterpolatorEntry * mem, uint8_t n)
    : _mem(mem), _tickrate(tickrate), _used(0), _cap(n), _taskstate(0)
{
    if(tickrate)
        start(tickrate);
}

void InterpolatorBase::_add(InterpolatorEntry & e)
{
    FGLCD_ASSERT(_used < _cap, "interpad"); // buffer full?
    fglcd::NoInterrupt no;
    _mem[_used++] = e;
}

InterpolatorBase::~InterpolatorBase()
{
    unregisterTask();
    waitTask();
}

void InterpolatorBase::tick()
{
    const uint8_t n = _used;
    for(uint8_t i = 0; i < n; ++i)
    {
        InterpolatorEntry e = _mem[i];
        e.tickfunc(e.iv);
    }
}

void _InterpolatedFancy::_finishTick()
{
    const Adj oldt = _t;
    if(oldt == Adj(-1) && _style == LERP_ONCE)
    {
        _style = OFF;
        return;
    }
    Adj tt = oldt + _speed; // will overflow if end reached

    if(tt <= oldt)
        switch(_style)  // overflow!
        {
            case LERP_ONCE:
                tt = Adj(-1); // one last time so we reach the end value
                break;
            case LERP_SAWTOOTH:
                tt = 0; // restart
                break;

        }

    _t = tt;
}

} // end namespace interp
