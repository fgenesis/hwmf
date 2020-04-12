#pragma once

// AVR specific include
// Warning: Inline asm ahead! The code the compiler would produce otherwise is hot garbage,
// fortunately with a bit of asm and poking it does the right thing and generates good enough code.

#if defined(_AVR_IOM8_H_) || defined(_AVR_IOM16_H_) || defined(_AVR_IOM32_H_)
#define FGLCD_IS_OLD_AVR
#endif

#ifndef FGLCD_IS_OLD_AVR
#define FGLCD_AVR_WRITE_PINX_TO_TOGGLE
#endif

#include <stddef.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <alloca.h>
#include "mcu.h"
#include "macros.h"
#include "mcu_avr_intrin.h"
#include "util.h"

#ifdef FGLCD_ASM
#define FGLCD_ASM_AVR
#endif


#ifdef RAMPZ
#  ifndef FGLCD_PROGMEM_FAR
#    define FGLCD_PROGMEM_FAR __attribute__((__section__(".fini0")))
#  endif
#  define fglcd_get_farptr(x) pgm_get_far_address(x)
//#  define PFSTR(s) (__extension__({static const char __c[] FGLCD_PROGMEM_FAR = (s); &__c[0];})) /* adapted from avr-libc */
#else
#  ifndef FGLCD_PROGMEM_FAR
#    define FGLCD_PROGMEM_FAR PROGMEM
#  endif
#  define fglcd_get_farptr(x) (reinterpret_cast<const unsigned char*>(x))
//#  define PFSTR(s) PSTR(s)
#endif

#define StackAlloc(x) alloca(x)

namespace fglcd {

struct RAMPZBackup
{
#ifdef RAMPZ
    const uint8_t zhh;
#endif
    FORCEINLINE RAMPZBackup()
#ifdef RAMPZ
        : zhh(RAMPZ)
#endif
    {}

    FORCEINLINE ~RAMPZBackup()
    {
#ifdef RAMPZ
        RAMPZ = zhh;
#endif
    }
};

#ifdef RAMPZ
typedef uint_farptr_t FarPtr;
#else
typedef const unsigned char* FarPtr;
#endif

typedef u8 unative;

#if 1
template<typename TY, uintptr_t aRP, uintptr_t aWP, uintptr_t aDDR>
struct Port : public PortBase<HardwareMMIO<TY, aRP, aWP, aDDR> >
{
    typedef PortBase<HardwareMMIO<TY, aRP, aWP, aDDR> > Base;
    typedef typename Base::IO IO;
#ifdef FGLCD_AVR_WRITE_PINX_TO_TOGGLE
    static FORCEINLINE void toggle(TY v) { IO::_writeIn(v); }
#endif // else use default impl

    // Write to MMIO regs explicitly as a hint for the compiler to use sbi/cbi instructions instead of read-modify-write
    template<unsigned bit>
    static FORCEINLINE void setbit() { *IO::hwWrite() |= TY(1u << bit); }
    template<unsigned bit>
    static FORCEINLINE void clearbit() { *IO::hwWrite() &= ~TY(1u << bit); }
};

#else

template<typename TY, uintptr_t aRP, uintptr_t aWP, uintptr_t aDDR>
struct Port
 {
    typedef bool is_port_tag;
    typedef TY value_type;

    static constexpr volatile TY * const hwWrite = (volatile TY*)aWP;
    static constexpr volatile TY * const hwRead = (volatile TY*)aRP;
    static constexpr volatile TY * const hwDDR = (volatile TY*)aDDR;
 
    static FORCEINLINE void set(TY v) { *hwWrite = v; }
    static FORCEINLINE TY get() { return *hwRead; }
    static FORCEINLINE TY readOutput() { return *hwWrite; }
    static FORCEINLINE void makeOutput(TY ddr = ~TY(0)) { *hwDDR = ddr; }
    static FORCEINLINE void makeInput(TY ddr = ~TY(0)) { makeOutput(~ddr); }
    static FORCEINLINE TY isOutput() { return *hwDDR; }
    static FORCEINLINE TY isInput() { return ~isOutput(); }

#ifdef FGLCD_AVR_WRITE_PINX_TO_TOGGLE
    static FORCEINLINE void toggle(TY v) { *hwRead = v; }
#endif // else use default impl

    // Write to MMIO regs explicitly as a hint for the compiler to use sbi/cbi instructions instead of read-modify-write
    template<unsigned bit>
    static FORCEINLINE void setbit() { *hwWrite |= value_type(1 << bit); }
    template<unsigned bit>
    static FORCEINLINE void clearbit() { *hwWrite &= ~value_type(1 << bit); }
};

#endif

template<typename PORT, typename PORT::value_type bit>
struct Pin : public PinBase<PORT, bit>
{
};

FORCEINLINE void delay_ms(u32 ms)
{
    _delay_ms(ms);
}
FORCEINLINE void delay_us(u32 us)
{
    _delay_us(us);
}
FORCEINLINE void delay_cyc(u32 cyc)
{
    __builtin_avr_delay_cycles(cyc);
}

struct NoInterrupt
{
    const u8 sreg;
    FORCEINLINE NoInterrupt() : sreg(SREG) { cli(); }
    FORCEINLINE ~NoInterrupt() { SREG = sreg; }
};

FORCEINLINE static uint8_t uhh8(uint32_t w)
{
    union layout
    {
        uint32_t i;
        struct { uint8_t lo, hi, hhi, hhhi; } b;
    } u;
    u.i = w;
    return u.b.hhi;
}



namespace memtype {

struct RAM
{
    typedef const unsigned char * Pointer;
    typedef const void * VoidPointer;
    typedef RAIINop SegmentBackup;

    template<typename T> static FORCEINLINE T read(const void *p, ptrdiff_t offs = 0) { return static_cast<const T*>(p)[offs]; }

    template<typename L>
    static FORCEINLINE void Memset_inl(void * dst, const uint8_t c, L len)
    {
#ifdef FGLCD_ASM_AVR
        FGLCD_DUFF8(L, len, {
            asm volatile(
                "st %a[dst]+,%[c]   \n\t"
                : [dst] "+x" ((volatile void*)dst) /*outputs*/
                : [c] "r" (c) /* inputs */
            );
        });
#else
        memset(dst, c, len);
#endif
    }

    static NOINLINE void Memset(void * dst, const uint8_t c, unsigned len)
    {
#ifdef FGLCD_ASM_AVR
        FGLCD_DUFF32(unsigned, len, {
            asm volatile(
                "st %a[dst]+,%[c]   \n\t"
                : [dst] "+x" ((volatile void*)dst) /*outputs*/
                : [c] "r" (c) /* inputs */
            );
        });
#else
        memset(dst, c, len);
#endif
    }

    template<typename Action, typename L>
    static FORCEINLINE void ForRange_inl(VoidPointer p, L sz, Action& a)
    {
#ifdef FGLCD_ASM_AVR
        uint8_t b;
        FGLCD_DUFF4(L, sz, {
            asm volatile(
                "ld %[b],%a[p]+   \n\t"
                : [b] "=&r" (b), [p] "+x" (p) /*outputs*/
                : /* inputs */
            );
            a(b);
        });
#else
        Pointer pp = (Pointer)p;
        while(sz--)
            a(*pp++);
#endif
    }

    template<typename Action>
    static NOINLINE void ForRange(VoidPointer begin, unsigned sz, Action& a)
    {
        ForRange_inl(begin, sz, a);
    }

    static NOINLINE void Memcpy(void *dst, VoidPointer src, unsigned len)
    {
#ifdef FGLCD_ASM_AVR
        FGLCD_DUFF32(unsigned, len, {
            asm volatile(
                "ld r0, %a[src]+    \n\t"
                "st %a[dst]+, r0    \n\t"
                : [dst] "+x" ((volatile void*)dst), [src] "+z" ((volatile void*)src) /*outputs*/
                : /* inputs */
                : "r0" /*clobber*/
            );
        });
#else
        memcpy(dst, src, len);
#endif
    }

    template<typename L>
    static FORCEINLINE void Memcpy_inl(void *dst, VoidPointer src, L len)
    {
#ifdef FGLCD_ASM_AVR
        FGLCD_DUFF4(L, len, {
            asm volatile(
                "ld r0, %a[src]+    \n\t"
                "st %a[dst]+, r0    \n\t"
                : [dst] "+x" ((volatile void*)dst), [src] "+z" ((volatile void*)src) /*outputs*/
                : /* inputs */
                : "r0" /*clobber*/
            );
        });
#else
    memcpy(dst, src, len);
#endif
    }

    static FORCEINLINE uint8_t readIncSafe(VoidPointer& p) { return readIncFast(p); }
    static FORCEINLINE uint8_t readIncSafe(Pointer& p) { return readIncFast(p); }

    static FORCEINLINE uint8_t readIncFast(Pointer& p) { return intrin::_avr_ld_postinc((const void**)&p); }
    static FORCEINLINE uint8_t readIncFast(VoidPointer& p) { return readIncFast((Pointer&)p); }

    static FORCEINLINE void storeIncFast(uint8_t *& p, uint8_t v) { return intrin::_avr_st_postinc((void**)&p, v); }
    static FORCEINLINE void storeIncFast(void*& p, uint8_t v) { storeIncFast((uint8_t*&)p, v); }

    static FORCEINLINE void SetSegmentPtr(Pointer p) {}
    static FORCEINLINE void fixFarPtr(VoidPointer& p) {}
    static FORCEINLINE void fixFarPtr(Pointer& p) {}
};

struct Progmem
{
    typedef const u8 * Pointer;
    typedef const void * VoidPointer;
    typedef RAIINop SegmentBackup;

    template<typename T> static FORCEINLINE T read(const void *p, ptrdiff_t offs = 0)
    {
        // Having this in a union also prevents calling any ctor
        union
        {
            T val;
            uint8_t buf[sizeof(T)];
        } u;
        const T *src = ((const T*)p) + offs;
        Memcpy_inl<typename TypeForSize<sizeof(T)>::type>(u.buf, src, sizeof(T));
        return u.val;
    }

    template<typename Action>
    static FORCEINLINE void ForRange_inl(VoidPointer p, unsigned sz, Action& a)
    {
#ifdef FGLCD_ASM_AVR
        uint8_t b;
        FGLCD_DUFF4(unsigned, sz, {
            asm volatile(
                "lpm %[b],%a[p]+   \n\t"
                : [b] "=&r" (b), [p] "+z" (p) /*outputs*/
                :  /* inputs */
            );
            a(b);
        });
#else
        Pointer pp = (Pointer)pp;
        while(sz--)
        {
            a(pgm_read_byte(pp));
            ++pp;
        }
#endif
    }

    template<typename Action>
    static NOINLINE void ForRange(VoidPointer pf, unsigned sz, Action& a)
    {
        ForRange_inl(pf, sz, a);
    }

    template<typename L>
    static FORCEINLINE void Memcpy_inl(void *dst, VoidPointer src, L sz)
    {
#ifdef FGLCD_ASM_AVR
        FGLCD_DUFF4(L, sz, {
            asm volatile(
                "lpm r0,%a[src]+  \n\t"
                "st %a[dst]+,r0   \n\t"
                : [src] "+z" (src), [dst] "+x" ((volatile void*)dst) /*outputs*/
                : /* inputs */
                : "r0" /* clobber */
            );
        });
#else
        memcpy_P(dst, src, sz);
#endif
    }
    static NOINLINE void Memcpy(void *dst, VoidPointer src, unsigned sz)
    {
#ifdef FGLCD_ASM_AVR
        FGLCD_DUFF32(unsigned, sz, {
            asm volatile(
                "lpm r0,%a[src]+  \n\t"
                "st %a[dst]+,r0   \n\t"
                : [src] "+z" (src), [dst] "+x" ((volatile void*)dst) /*outputs*/
                : /* inputs */
                : "r0" /* clobber */
            );
        });
#else
        memcpy_P(dst, src, sz);
#endif
    }

    static FORCEINLINE uint8_t readIncSafe(VoidPointer& p) { return readIncFast(p); }
    static FORCEINLINE uint8_t readIncSafe(Pointer& p) { return readIncFast(p); }

    static FORCEINLINE uint8_t readIncFast(VoidPointer& p) { return intrin::_avr_lpm_postinc(&p); }
    static FORCEINLINE uint8_t readIncFast(Pointer& p) { return readIncFast((VoidPointer&)p); }

    static FORCEINLINE void SetSegmentPtr(FarPtr) {}
    static FORCEINLINE void fixFarPtr(Pointer& p) {}
    static FORCEINLINE void fixFarPtr(VoidPointer& p) {}
};
#ifdef RAMPZ
struct ProgmemFar
{
    typedef FarPtr Pointer;
    typedef FarPtr VoidPointer;
    typedef RAMPZBackup SegmentBackup;

    template<typename T> static T read(FarPtr p, ptrdiff_t offs = 0)
    {
        // Having this in a union also prevents calling any ctor
        union
        {
            T val;
            uint8_t buf[sizeof(T)];
        } u;
        FarPtr src = p + (offs * sizeof(T));
        Memcpy_inl<typename TypeForSize<sizeof(T)>::type>(u.buf, src, sizeof(T));
        return u.val;
    }

    template<typename Action, typename L>
    static FORCEINLINE void ForRange_inl(VoidPointer pf, L sz, Action& a)
    {
        SegmentBackup seg;
        RAMPZ = uhh8(pf);
        uint8_t b;
        void *p = (void*)pf;
        FGLCD_DUFF4(L, sz, {
            asm volatile(
                "elpm %[b],%a[p]+   \n\t"
                : [b] "=&r" (b), [p] "+z" ((volatile void*)p) /*outputs*/
                :  /* inputs */
            );
            a(b);
        });
    }

    template<typename Action>
    static NOINLINE void ForRange(VoidPointer pf, unsigned sz, Action& a)
    {
        ForRange_inl(pf, sz, a);
    }

    template<typename L>
    static FORCEINLINE void Memcpy_inl(void *dst, VoidPointer srcf, L sz)
    {
        SegmentBackup seg;
        RAMPZ = uhh8(srcf);
        void *src = (void*)srcf;
        FGLCD_DUFF4(L, sz, {
            asm volatile(
                "elpm r0,%a[src]+  \n\t"
                "st %a[dst]+,r0   \n\t"
                : [src] "+z" (src), [dst] "+x" ((volatile void*)dst) /*outputs*/
                : /* inputs */
                : "r0" /* clobber */
            );
        });
    }
    static NOINLINE void Memcpy(void *dst, VoidPointer srcf, unsigned sz)
    {
        SegmentBackup seg;
        RAMPZ = uhh8(srcf);
        void *src = (void*)srcf;
        FGLCD_DUFF32(unsigned, sz, {
            asm volatile(
                "elpm r0,%a[src]+  \n\t"
                "st %a[dst]+,r0   \n\t"
                : [src] "+z" (src), [dst] "+x" ((volatile void*)dst) /*outputs*/
                : /* inputs */
                : "r0" /* clobber */
            );
        });
    }

    static FORCEINLINE uint8_t readIncSafe(VoidPointer& p)
    {
        SetSegmentPtr(p);
        const uint8_t c = readIncFast(p);
        fixFarPtr(p);
        return c;
    }

    static FORCEINLINE void SetSegmentPtr(VoidPointer p)
    {
        RAMPZ = uhh8(p);
    }

    // set RAMPZ before calling this!
    static FORCEINLINE uint8_t readIncFast(VoidPointer& p)
    {
        return intrin::_avr_elpm_postinc((const void**)&p);
    }

    static FORCEINLINE void fixFarPtr(VoidPointer& p)
    {
        union
        {
            Pointer pp;
            const void *lo;
            uint8_t c[sizeof(Pointer)];
        };
        pp = (Pointer)p;
        c[3] = RAMPZ;
    }
};
#else
    typedef Progmem ProgmemFar;
#endif
} // end namespace memtype


// ok, this used to work in an older avr-gcc but starting from 6.0 it's barfing all over the place
// so use that namespace trick to make it obey
namespace _avrportdef {
#define _EXPAND(x) x // freaky indirection to make sure the ## expansion happens before macro expansion
#define AVRPORT(_, pin, port, ddr) \
    static const uintptr_t _EXPAND(pin##_ADDR) = _SFR_MEM_ADDR(pin); \
    static const uintptr_t _EXPAND(port##_ADDR) = _SFR_MEM_ADDR(port); \
    static const uintptr_t _EXPAND(ddr##_ADDR) = _SFR_MEM_ADDR(ddr);
#include "mcu_avr_portdef.h"
#undef AVRPORT
}
#define AVRPORT(name, pin, port, ddr) \
    typedef fglcd::Port<u8, \
        _avrportdef::_EXPAND(pin##_ADDR), \
        _avrportdef::_EXPAND(port##_ADDR), \
        _avrportdef::_EXPAND(ddr##_ADDR) \
    > name;
#include "mcu_avr_portdef.h"
#undef AVRPORT
#undef _EXPAND

struct InsideInterrupt {};


FORCEINLINE static uint8_t ulo8(uint16_t w)
{
    return uint8_t(w);
}

FORCEINLINE static uint8_t uhi8(uint16_t w)
{
    union layout
    {
        uint16_t i;
        struct { uint8_t lo, hi; } b;
    } u;
    u.i = w;
    return u.b.hi;
}

FORCEINLINE static uint16_t make16(uint8_t lo, uint8_t hi)
{
    union layout
    {
        uint16_t i;
        struct { uint8_t lo, hi; } b;
    } u;
    u.b.lo = lo;
    u.b.hi = hi;
    return u.i;
}

#if 0

namespace SPI {
    enum DataOrder
    {
        MSB_FIRST = 0,
        LSB_FIRST = 1,
    };
    inline void enable(DataOrder ord)
    {
        SPCR |= _BV(SPE) | (ord << DORD);
        SPSR |= _BV(SPI2X);
    }
    FORCEINLINE void wait()
    {
        while(!(SPSR & (1<<SPIF))) {}
    }
    FORCEINLINE void send(uint8_t x)
    {
        SPDR = x;
    }
    FORCEINLINE void waitSend(uint8_t x)
    {
        wait();
        send(x);
    }
    FORCEINLINE void sendWait(uint8_t x)
    {
        send(x);
        wait();
    }
}

inline static void enableXMEM()
{
    XMCRB = 0;//_BV(XMBK);
    XMCRA = _BV(SRE);
}

inline static void disableXMEM()
{
    XMCRA = 0;
    XMCRB = 0;
}

static u16 xmem_lastval;

struct _Connection_XMEM_16
{
    typedef u16 value_type;
    
    static FORCEINLINE void set(u16 v) { xmem_lastval = v; }
    static FORCEINLINE void latch() { send(xmem_lastval); }
    static FORCEINLINE void send(u16 v) { *((volatile u8*)(uintptr_t)v); }
};

struct  _Connection_XMEM_8
{
    typedef u8 value_type;
    static FORCEINLINE void set(u16 v) { xmem_lastval = (v|0xff00); }
    static FORCEINLINE void latch() { send(xmem_lastval); }
    static FORCEINLINE void send(u16 v) { *((volatile u8*)(uintptr_t)(v|0xff00)); }
};

template<typename DUMMYPORT, typename DUMMYTRIGGER>
struct Connection_XMEM_8 : public _Connection_XMEM_8
{
};

template<typename DUMMYPORT, typename DUMMYTRIGGER>
struct Connection_XMEM_16: public _Connection_XMEM_16
{
};
#endif


} // end namespace fglcd
