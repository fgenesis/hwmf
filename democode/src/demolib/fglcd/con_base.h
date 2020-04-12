#pragma once

#include "trigger.h"

namespace fglcd {

// The actual connection. Con is used to set data, trigger is fired to notify external hardware that new data were set.
template<typename Con, typename TRIGGER>
struct Connection
{
    typedef bool is_connection_tag;
    typedef TRIGGER Trigger;
    static_assert(has_con_tag<Con>::value, "Con is not a con");
    static_assert(has_trigger_tag<Trigger>::value, "Trigger is not a trigger");
    typedef typename Con::value_type value_type;

    static FORCEINLINE void _latch()                    { Trigger::trigger(); }
    static FORCEINLINE void send(value_type v)          { Con::send(v); _latch(); }
    static FORCEINLINE void send()                      { Con::send(); _latch(); }
    static FORCEINLINE void set(value_type v)           { Con::set(v); }
    static FORCEINLINE void sendSameAgain(value_type v) { Con::sendSameAgain(v); _latch(); }
    static FORCEINLINE value_type readOutput()          { return Con::readOutput(); }
};

// Connection config: Send data via a (parallel) port, define (but not used in this class) a trigger.
// (The trigger is further specialized by the Chip<> class -- hold times and so on)
template<typename PORT, typename TRIGGERPIN>
struct Con_Port
{
    typedef bool is_con_tag;
    typedef PORT Port;
    typedef TRIGGERPIN TriggerPin; // Used by Connection<>
    static_assert(has_port_tag<Port>::value, "Port is not a port");
    static_assert(has_pin_tag<TriggerPin>::value, "TriggerPin is not a pin");
    typedef typename Port::value_type value_type;

    static FORCEINLINE void set(value_type v) { Port::set(v); }
    static FORCEINLINE void send(value_type v) { Port::set(v); }
    static FORCEINLINE void send() {} // nothing to do
    static FORCEINLINE void sendSameAgain(value_type) {} // nothing to do
    static FORCEINLINE value_type readOutput() { return Port::readOutput(); }
};

enum SPIConWaitSendOrder
{
    SPI_CON_WAIT_THEN_SEND,
    SPI_CON_SEND_THEN_WAIT,
    SPI_CON_SEND
};

/*
template<SPIConWaitSendOrder> struct SPISender;
template<> struct SPISender<SPI_CON_WAIT_THEN_SEND> { FORCEINLINE void send(unative x) { SPI::waitSend(x); } };
template<> struct SPISender<SPI_CON_SEND_THEN_WAIT> { FORCEINLINE void send(unative x) { SPI::sendWait(x); } };
template<> struct SPISender<SPI_CON_SEND>           { FORCEINLINE void send(unative x) { SPI::send(x); } };

// SPI connection
template<typename Ty, SPIConWaitSendOrder>
struct Con_SPI;

template<SPIConWaitSendOrder ord>
struct Con_SPI<u8, ord>
{
    typedef bool is_con_tag;
    typedef u8 value_type;
    typedef DummyPin TriggerPin; // Used by Connection<>
    typedef SPISender<ord> Send;
    static value_type _last;
    static FORCEINLINE void set(value_type v) { _last = v; }
    static FORCEINLINE void send() { send(_last); }
    static FORCEINLINE void send(value_type v) { Send::send(v); }
    static FORCEINLINE void sendSameAgain(value_type v) { send(v); }
    static FORCEINLINE value_type readOutput() { return _last; }
};

template<SPIConWaitSendOrder ord>
struct Con_SPI<u16, ord>
{
    typedef bool is_con_tag;
    typedef u16 value_type;
    typedef DummyTrigger TriggerPin; // Used by Connection<>
    typedef SPISender<ord> Send;
    static value_type _last;
    static FORCEINLINE void set(value_type v) { _last = v; }
    static FORCEINLINE void send() { send(_last); }
    static FORCEINLINE void send(value_type v) { Send::send(ulo8(v)); Send::send(uhi8(v)); }
    static FORCEINLINE void sendSameAgain(value_type v) { send(v); }
    static FORCEINLINE value_type readOutput() { return _last; }
};
*/

} // end namespace fglcd
