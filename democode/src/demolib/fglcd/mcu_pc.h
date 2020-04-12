#pragma once

#ifndef NO_SDL
#include <SDL.h>
#endif

#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "util.h"

#pragma warning(disable: 4530)
#pragma warning(disable: 4577)

#define MCU_IS_PC

#define PROGMEM
#define FGLCD_PROGMEM_FAR

#define PSTR(s) (s)
#define PFSTR(s) (s)

#define snprintf_P snprintf
#define strncpy_P strncpy

#define fglcd_get_farptr(x) (reinterpret_cast<fglcd::FarPtr>(&x[0]))

#define StackAlloc(x) alloca(x) // TODO: some fancy thing?

#if defined(unix) && (defined(__i386__) || defined(__x86_64__))
static inline void debug_break(void) {
    __asm__ volatile("int $0x03");
}

#elif defined(_WIN32)
# define debug_break __debugbreak

#else
# error "No breakpoint implementation defined"
#endif

#ifndef NO_SDL
extern SDL_mutex *g_interruptMutex;
#endif


namespace fglcd {

typedef const unsigned char* FarPtr;
typedef unsigned unative;

template<typename TY, unsigned portnum>
struct Port : public PortBase<EmulatedMMIO<TY, portnum> >
{
};

template<typename PORT, typename PORT::value_type bit>
struct Pin : public PinBase<PORT, bit>
{
};


namespace memtype {

struct RAM
{
    typedef const unsigned char * Pointer;
    typedef const void * VoidPointer;
    typedef RAIINop SegmentBackup;
    template<typename T> static FORCEINLINE T read(const void *p, ptrdiff_t offs = 0) { return static_cast<const T*>(p)[offs]; }

    template<typename L>
    static FORCEINLINE void Memcpy_inl(void *dst, VoidPointer src, L len) { memcpy(dst, src, len); }
    static FORCEINLINE void Memcpy(void *dst, VoidPointer src, size_t len) { memcpy(dst, src, len); }

    template<typename L>
    static FORCEINLINE void Memset_inl(void *dst, const uint8_t c, L len) { memset(dst, c, len); }
    static FORCEINLINE void Memset(void *dst, const uint8_t c, size_t len) { memset(dst, c, len); }

    template<typename Action>
    static FORCEINLINE void ForRange_inl(VoidPointer begin, size_t sz, Action& a)
    {
        Pointer p = (Pointer)begin;
        Pointer const end = p + sz;
        while(p < end)
            a(*p++);
    }

    template<typename Action>
    static NOINLINE void ForRange(VoidPointer begin, size_t sz, Action& a)
    {
        ForRange_inl(begin, sz, a);
    }

    static FORCEINLINE uint8_t readIncSafe(VoidPointer& p) { return readIncFast(p); }
    static FORCEINLINE uint8_t readIncSafe(Pointer& p) { return readIncFast(p); }

    static FORCEINLINE void SetSegmentPtr(FarPtr p) {}
    static FORCEINLINE uint8_t readIncFast(Pointer& p) { return *p++; }
    static FORCEINLINE uint8_t readIncFast(VoidPointer& p) { return readIncFast((Pointer&)p); }
    static FORCEINLINE void fixFarPtr(VoidPointer& p) {}
    static FORCEINLINE void fixFarPtr(Pointer& p) {}
    static FORCEINLINE void storeIncFast(uint8_t*& p, uint8_t v) { *p++ = v; }
    static FORCEINLINE void storeIncFast(void*& p, uint8_t v) { storeIncFast((uint8_t*&)p, v); }
};

typedef RAM Progmem;
typedef RAM ProgmemFar;
}

struct SegmentBackup
{
    FORCEINLINE SegmentBackup() {}
    FORCEINLINE ~SegmentBackup() {}
};

FORCEINLINE static uint16_t make16(uint8_t lo, uint8_t hi)
{
    return (hi << 8u) | lo;
}

FORCEINLINE static uint8_t ulo8(uint16_t w)
{
    return uint8_t(w);
}

FORCEINLINE static uint8_t uhi8(uint16_t w)
{
    return uint8_t(w >> 8);
}

FORCEINLINE static uint8_t uhh8(uint32_t w)
{
    return uint8_t(w >> 16);
}



FORCEINLINE void delay_ms(u32 ms)
{
#ifndef NO_SDL
    SDL_Delay(ms);
#endif
}
FORCEINLINE void delay_us(u32 us)
{
    // too short anyway
}
FORCEINLINE void delay_cyc(u32 cyc)
{
    // lolnope
}

struct _SDLGlobalLock
{
    FORCEINLINE _SDLGlobalLock()
    {
#ifndef NO_SDL
        SDL_LockMutex(g_interruptMutex);
#endif
    }
    FORCEINLINE ~_SDLGlobalLock()
    {
#ifndef NO_SDL
        SDL_UnlockMutex(g_interruptMutex);
#endif
    }
};

typedef _SDLGlobalLock NoInterrupt;
typedef _SDLGlobalLock InsideInterrupt;


typedef fglcd::Port<uint8_t, 0> PortA;
typedef fglcd::Port<uint8_t, 1> PortB;
typedef fglcd::Port<uint8_t, 2> PortC;
typedef fglcd::Port<uint8_t, 3> PortD;
typedef fglcd::Port<uint8_t, 4> PortE;
typedef fglcd::Port<uint8_t, 5> PortF;
typedef fglcd::Port<uint8_t, 6> PortG;
typedef fglcd::Port<uint8_t, 7> PortH;
typedef fglcd::Port<uint8_t, 8> PortI;
typedef fglcd::Port<uint8_t, 9> PortJ;
typedef fglcd::Port<uint8_t, 10> PortK;
typedef fglcd::Port<uint8_t, 11> PortL;


} // end namespace fglcd



