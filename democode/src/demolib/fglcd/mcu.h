#pragma once

#include "def.h"
#include "macros.h"


// functions for software port emulation
extern void mcu_out_port_write(unsigned portnum, unsigned value);
extern unsigned mcu_out_port_read(unsigned portnum);
extern void mcu_in_port_write(unsigned portnum, unsigned value);
extern unsigned mcu_in_port_read(unsigned portnum);
extern void mcu_ddr_port_write(unsigned portnum, unsigned value);
extern unsigned mcu_ddr_port_read(unsigned portnum);

namespace fglcd {

template<typename TY, uintptr_t aRP, uintptr_t aWP, uintptr_t aDDR>
struct HardwareMMIO
{
    typedef TY value_type;

    // this doesn't work due to gcc 6+ bug
    // ref: https://stackoverflow.com/questions/24398102/constexpr-and-initialization-of-a-static-const-void-pointer-with-reinterpret-cas
    /*static constexpr const _hwWrite = (volatile TY*)aWP;
    static constexpr const _hwRead = (volatile TY*)aRP;
    static constexpr const _hwDDR = (volatile TY*)aDDR;*/

    // workaround
    static constexpr const uintptr_t _hwWrite = aWP;
    static constexpr const uintptr_t _hwRead = aRP;
    static constexpr const uintptr_t _hwDDR = aDDR;
    static FORCEINLINE volatile TY *hwRead() { return reinterpret_cast<volatile TY*>(_hwRead); }
    static FORCEINLINE volatile TY *hwWrite() { return reinterpret_cast<volatile TY*>(_hwWrite); }
    static FORCEINLINE volatile TY *hwDDR() { return reinterpret_cast<volatile TY*>(_hwDDR); }
    
    static FORCEINLINE void _writeOut(TY v) { *hwWrite() = v; }
    static FORCEINLINE TY _readOut() { return *hwWrite(); }
    static FORCEINLINE void _writeIn(TY v) { *hwRead() = v; }
    static FORCEINLINE TY _readIn() { return *hwRead(); }
    static FORCEINLINE void _writeDDR(TY v) { *hwDDR() = v; }
    static FORCEINLINE TY _readDDR() { return *hwDDR(); }
};

template<typename TY, unsigned portnum>
struct EmulatedMMIO
{
    typedef TY value_type;

    static FORCEINLINE void _writeOut(value_type v) { mcu_out_port_write(portnum, v); }
    static FORCEINLINE value_type _readOut() { return (value_type)mcu_out_port_read(portnum); }
    static FORCEINLINE void _writeIn(value_type v) { mcu_in_port_write(portnum, v); }
    static FORCEINLINE value_type _readIn() { return (value_type)mcu_in_port_read(portnum); }
    static FORCEINLINE void _writeDDR(value_type v) { mcu_ddr_port_write(portnum, v); }
    static FORCEINLINE value_type _readDDR() { return (value_type)mcu_ddr_port_read(portnum); }
};

template<typename IO_>
struct PortBase
{
    typedef bool is_port_tag;
    typedef IO_ IO;
    typedef typename IO::value_type value_type;

    static FORCEINLINE void set(value_type v) { IO::_writeOut(v); }
    static FORCEINLINE value_type get() { return IO::_readIn(); }
    static FORCEINLINE value_type readOutput() { return IO::_readOut(); }
    static FORCEINLINE void makeOutput(value_type ddr = ~value_type(0)) { IO::_writeDDR(ddr); }
    static FORCEINLINE void makeInput(value_type ddr = ~value_type(0)) { makeOutput(~ddr); }
    static FORCEINLINE value_type isOutput() { return IO::_readDDR(); }
    static FORCEINLINE value_type isInput() { return ~isOutput(); }
    static FORCEINLINE void toggle(value_type v) { set(get() ^ v); }

    template<unsigned bit>
    static FORCEINLINE void setbit() { set(get() | value_type(bit << 1)); }
    template<unsigned bit>
    static FORCEINLINE void clearbit() { set(get() & ~value_type(bit << 1)); }
};

/*
template<typename TY, uintptr_t aRP, uintptr_t aWP, uintptr_t aDDR>
struct HardwarePort : public PortBase<HardwareMMIO<TY, aRP, aWP, aDDR> >
{
    typedef HardwareMMIO<TY, aRP, aWP, aDDR> MMIO;
    typedef PortBase<MMIO> Base;
};
*/


template<typename PORT, typename PORT::value_type bit>
struct PinBase
{
    typedef bool is_pin_tag;
    typedef PORT Port;
    typedef typename PORT::value_type value_type;
    enum { mask = 1 << bit };

    static FORCEINLINE void hi() { Port::template setbit<bit>(); }
    static FORCEINLINE void lo() { Port::template clearbit<bit>(); }
    //static FORCEINLINE void set(bool v) { if(get() != v) toggle(); }
    static FORCEINLINE void set(bool v) { if(v) hi(); else lo(); } // safer
    static FORCEINLINE bool get() { return !!(Port::get() & mask); }
    static FORCEINLINE void toggle() { Port::toggle(mask); }
    static FORCEINLINE void pulse_lo() { lo(); hi(); }
    static FORCEINLINE void pulse_hi() { hi(); lo(); }
    static FORCEINLINE void pulse() { toggle(); toggle(); }
    static FORCEINLINE void makeOutput(bool out = true) { Port::makeOutput(Port::isOutput() | (value_type(out) << bit)); }
    static FORCEINLINE void makeInput(bool in = true) { makeOutput(!in); }
};

} // end namespace fglcd

#ifdef __AVR__
#include "mcu_avr.h"
#elif defined(_M_IX86) || defined(_WIN32) || defined(unix)
#include "mcu_pc.h"
#else
#error unknown mcu
#endif

namespace fglcd {

using memtype::RAM;
using memtype::Progmem;
using memtype::ProgmemFar;

template<typename TY = unative, TY readval = 0>
struct DummyPortBase
{
    typedef bool is_port_tag;
    typedef TY value_type;
    static FORCEINLINE void set(TY v) {}
    static FORCEINLINE TY get() { return readval; }
    static FORCEINLINE void makeOutput(TY) {}
    static FORCEINLINE void makeInput(TY) {}
    static FORCEINLINE void toggle(TY) {}
    static FORCEINLINE TY isOutput() { return 0; }
    static FORCEINLINE TY isInput() { return 0; }

    template<unsigned bit>
    static FORCEINLINE void setbit() {}
    template<unsigned bit>
    static FORCEINLINE void clearbit() {}
};

struct DummyPort : public DummyPortBase<unative, 0>
{
};

struct DummyPin : PinBase<DummyPort, 0>
{
};

// Merges two distinct ports into a single virtual port that has the size of both ports combined
template<typename TY, typename Port1, typename Port2>
struct Port2x
{
    typedef bool is_port_tag;
    typedef TY value_type;
    typedef typename Port1::value_type hw_type;
    typedef typename Port2::value_type hw_type2;
    //static_assert(is_same<hw_type, hw_type2>::value, "Ports must have same underlying type");
    static_assert(sizeof(value_type) == sizeof(hw_type) + sizeof(hw_type2), "Combined type must be of same length than both hardware ports");

    union Split
    {
        struct { hw_type lo; hw_type2 hi; } hw;
        value_type both;
    };
    static FORCEINLINE void set(TY v) { Split s; s.both = v; Port1::set(s.hw.lo); Port2::set(s.hw.hi); }
    static FORCEINLINE TY get() { Split s; s.hw.lo = Port1::get(); s.hw.hi = Port2::get(); return s.both; }
    static FORCEINLINE void makeOutput(TY ddr = TY(-1)) { Split s; s.both = ddr; Port1::makeOutput(s.hw.lo); Port2::makeOutput(s.hw.hi); }
    static FORCEINLINE void toggle(TY v) {  Split s; s.both = v; Port1::toggle(s.hw.lo); Port2::toggle(s.hw.hi); }
    static FORCEINLINE TY readOutput() { Split s; s.hw.lo = Port1::readOutput(); s.hw.hi = Port2::readOutput(); return s.both; }
};

// Merges parts of 2 ports into a single virtual port that takes some bits from each port
// Warning: Port setting is dirty -- unused bits in each port are cleared to 0
template<typename TY, typename Port1, TY Mask1, typename Port2, TY Mask2>
struct SplitPort_Dirty
{
    typedef bool is_port_tag;
    typedef TY value_type;
    typedef typename Port1::value_type hw_type;
    typedef typename Port2::value_type hw_type2;
    //static_assert(is_same<hw_type, hw_type2>::value, "Ports must have same underlying type");
    static_assert(sizeof(value_type) == sizeof(hw_type), "Combined type must be of same length as each hardware port");
    static_assert(sizeof(value_type) == sizeof(hw_type2), "Combined type must be of same length as each hardware port");

    static FORCEINLINE void set(TY v) { Port1::set(v & Mask1); Port2::set(v & Mask2); }
    static FORCEINLINE TY get() { return (Port1::get() & Mask1) | (Port2::get() & Mask2); }
    static FORCEINLINE void makeOutput(TY ddr = TY(-1)) { Port1::makeOutput(ddr & Mask1); Port2::makeOutput(ddr & Mask2); }
    static FORCEINLINE void toggle(TY v) {  Port1::toggle(v & Mask1); Port2::toggle(v & Mask2); }
    static FORCEINLINE TY readOutput() { return (Port1::readOutput() & Mask1) | (Port2::readOutput() & Mask2); }
};



} // end namespace fglcd

