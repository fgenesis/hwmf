#pragma once

#include "mcu.h"

namespace fglcd {

template<typename PIN>
struct _TriggerBase
{
    typedef PIN Pin;
    //static_assert(has_pin_tag<Pin>::value, "Pin is not a pin");
    typedef bool is_trigger_tag;
};

struct DummyTrigger : public _TriggerBase<DummyPin>
{
    static FORCEINLINE void trigger() {}
    static FORCEINLINE void set() {}
    static FORCEINLINE void clear() {}
};

template<typename PIN>
struct ToggleTrigger : public _TriggerBase<PIN>
{
    static FORCEINLINE void trigger() { PIN::pulse(); }
    static FORCEINLINE void set() { PIN::toggle(); }
    static FORCEINLINE void clear() { PIN::toggle(); }
};

template<typename PIN>
struct LowTrigger : public _TriggerBase<PIN>
{
    static FORCEINLINE void trigger() { PIN::pulse_lo(); }
    static FORCEINLINE void set() { PIN::lo(); }
    static FORCEINLINE void clear() { PIN::hi(); }
};

template<typename PIN>
struct HighTrigger : public _TriggerBase<PIN>
{
    static FORCEINLINE void trigger() { PIN::pulse_hi(); }
    static FORCEINLINE void set() { PIN::hi(); }
    static FORCEINLINE void clear() { PIN::lo(); }
};

// ------------------
// Derived triggers -- take existing trigger, add hold times

template<typename Trig>
struct TriggerPulse : public Trig
{
    //static_assert(has_trigger_tag<Trig>::value, "Trig is not a trigger");
};

template<typename Trig, u32 ms>
struct TriggerHoldMS : public Trig
{
    //static_assert(has_trigger_tag<Trig>::value, "Trig is not a trigger");
    static FORCEINLINE void trigger() { set(); clear(); }
    static FORCEINLINE void set() { Trig::set(); delay_ms(ms); }
    static FORCEINLINE void clear() { Trig::clear(); }
};

template<typename Trig, u32 cyc>
struct TriggerHoldCycles : public Trig
{
    //static_assert(has_trigger_tag<Trig>::value, "Trig is not a trigger");
    static FORCEINLINE void trigger() { set(); clear(); }
    static FORCEINLINE void set() { Trig::set(); delay_cyc(cyc); }
    static FORCEINLINE void clear() { Trig::clear(); }
};

template<typename Trig, u32 ms>
struct TriggerWaitMS : public Trig
{
    //static_assert(has_trigger_tag<Trig>::value, "Trig is not a trigger");
    static FORCEINLINE void trigger() { set(); clear(); }
    static FORCEINLINE void set() { Trig::set(); }
    static FORCEINLINE void clear() { Trig::clear(); delay_ms(ms); }
};



} // end namespace fglcd
