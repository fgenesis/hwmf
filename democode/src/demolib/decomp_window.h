#pragma once

#include "decomp_def.h"
#include "decomp_util.h"

#include "scratch.h"


// "Side-effect" when emitting bytes

struct DecompToRAM // no extra side effect
{
    static FORCEINLINE void emitSingleByte(uint8_t c) {}
    static FORCEINLINE void emitMultiByte(uint8_t c, unsigned len) {}
};

template<> struct SelectSideEffect<ToRAM> { typedef DecompToRAM type; };



// Class to handle decompressor window

template<DecompWindowType, typename SideEffect>
struct DecompEmitterWindow;

template<typename SideEffect>
struct DecompEmitterWindow<DECOMP_WINDOW_NONE, SideEffect> // No window -> suitable for RLE or RRR
{
    struct Action
    {
        FORCEINLINE Action(const SideEffect& eff_) : eff(eff_) {}
        FORCEINLINE void operator()(uint8_t x) { eff.emitSingleByte(x); }
        SideEffect eff;
    };
    Action _a;
    FORCEINLINE DecompEmitterWindow(void * = NULL, unsigned = 0, const SideEffect& eff = SideEffect()) : _a(eff) {}
    FORCEINLINE void emitSingleByte(uint8_t c) { _a(c); }
    FORCEINLINE void emitMultiByte(uint8_t c, unsigned len) { _a.eff.emitMultiByte(c, len); }

    template<typename Reader>
    FORCEINLINE void emitLiterals(typename Reader::Pointer rd, unsigned len)
    {
        //FGLCD_ASSERT((uintptr_t)rd < (uintptr_t)rd + len, "gotcha");
        Reader::ForRange_inl(rd, len, _a);
    }
    //void emitOffset(unsigned offset, unsigned len); // does not exist without a window
};

template<typename SideEffect>
struct DecompEmitterWindow<DECOMP_WINDOW_OUTPUT, SideEffect>
{
    struct Action
    {
        FORCEINLINE Action(const SideEffect& eff_, uint8_t *p_) : p(p_), eff(eff_) {}
        FORCEINLINE void operator()(uint8_t x) { fglcd::RAM::storeIncFast(p, x); eff.emitSingleByte(x); }
        uint8_t *p;
        SideEffect eff;
    };
    Action _a;
    FORCEINLINE DecompEmitterWindow(void *pp, unsigned = 0, const SideEffect& eff = SideEffect()) : _a(eff, (uint8_t*)pp) {}
    FORCEINLINE void emitSingleByte(uint8_t c) { _a(c); }
    FORCEINLINE void emitMultiByte(uint8_t c, unsigned len)
    {
        _a.eff.emitMultiByte(c, len);
        fglcd::RAM::Memset(_a.p, c, len);
        _a.p += len;
    }

    template<typename Reader>
    FORCEINLINE void emitLiterals(typename Reader::Pointer rd, unsigned len)
    {
        Reader::ForRange_inl(rd, len, _a);
    }

    void emitOffset(unsigned offset, unsigned len)
    {
        const uint8_t *rd = _a.p - offset;

        if(offset == 1)
            emitMultiByte(*rd, len);
        else
            this->template emitLiterals<fglcd::RAM>(rd, len);
    }
};

/*
// WARNING UNFINISHED + UNTESTED!
template<typename SideEffect>
struct DecompEmitterWindow<DECOMP_WINDOW_BIG, SideEffect>
{
    uint8_t *p;
    uint8_t * const pbegin;
    uint8_t * const pend;
    const unsigned  size;
    FORCEINLINE void inc(uint8_t *& pp, unsigned x) { pp += x; while(pp >= pend) pp -= size; }
    FORCEINLINE unsigned space(const uint8_t *pp) const { return pend - pp; }
    FORCEINLINE DecompEmitterWindow(uint8_t *pp, unsigned sz) : p(pp), pbegin(pp), pend(pp + sz), size(sz) {}
    // initial condition: pbegin <= p < pend
    // aka p always points to the next valid spot to write to
    FORCEINLINE void emitSingleByte(uint8_t c) { *p = c; inc(p, 1); }
    FORCEINLINE void emitMultiByte(uint8_t c, unsigned len)
    {
        do
        {
            const unsigned n = vmin(space(p), len);
            memset(p, c, n);
            inc(p, n);
            len -= n;
        }
        while(len);
    }
    template<typename Reader>
    FORCEINLINE void emitLiterals(typename Reader::Pointer buf, unsigned len)
    {
        Reader::SetSegmentPtr(buf);
        do
        {
            const unsigned n = vmin(space(p), len);
            Reader::Memcpy(p, buf, len);
            inc(p, n);
            len -= n;
        }
        while(len);
    }

    FORCEINLINE void emitOffset(unsigned offset, unsigned len)
    {
        uint8_t *rd = p - offset;
        if(rd < pbegin)
            rd += space(pbegin); // wrap around backwards
        do
        {
            const unsigned n = vmin3(space(p), space(rd), len);
            memmove(p, rd, n);
            inc(p, n);
            inc(rd, n);
            len -= n;
        }
        while(len);
    }
};
*/

// must be used with page-aligned ptr and exactly 256b buffer size
template<typename SideEffect>
struct DecompEmitterWindow<DECOMP_WINDOW_TINY, SideEffect>
{
    enum { WINDOWSIZE = 256 };
    union Up8
    {
        uint8_t *p;
        uint8_t c[sizeof(uint8_t*)];
    };

    struct Action
    {
        FORCEINLINE Action(const SideEffect& eff_, uint8_t *p) : eff(eff_)
        {
            u.p = p ? p : CFG_PTR_DECOMPWINDOW;
            FGLCD_ASSERT(!(uintptr_t(p) & (WINDOWSIZE-1)), "wtinyaln");
        }
        FORCEINLINE void operator()(uint8_t x)
        {
            eff.emitSingleByte(x);
            *u.p = x;
            ++u.c[0];
        }
        Up8 u;
        SideEffect eff;
    };
    Action _a;

    FORCEINLINE DecompEmitterWindow(void *p, unsigned = 0, const SideEffect& eff = SideEffect())
        : _a(eff, (uint8_t*)p)
    {}

    FORCEINLINE void emitSingleByte(uint8_t c) { _a(c); }

    void emitMultiByte(uint8_t c, unsigned len)
    {
        _a.eff.emitMultiByte(c, len);

        if(uhi8(len)) // filling more than the window; just fill everything
        {
            _a.u.c[0] = 0;
            fglcd::RAM::Memset(_a.u.p, c, WINDOWSIZE);
        }
        else
        {
            uint8_t sz = uint8_t(len);
            uint8_t rem = uint8_t(0xff) - _a.u.c[0] + uint8_t(1); // space until hitting end of page
            if(rem && sz > rem) // remaining space until page end
            {
                fglcd::RAM::Memset(_a.u.p, c, rem);
                _a.u.c[0] = 0; // restart
                sz -= rem;
            }
            if(sz) // whatever goes into new page
            {
                fglcd::RAM::Memset(_a.u.p, c, sz);
                _a.u.c[0] += sz;
            }
        }
    }

    template<typename Reader>
    FORCEINLINE void emitLiterals(typename Reader::Pointer rd, unsigned len)
    {
        Reader::ForRange_inl(rd, len, _a);
    }

    void emitOffset(unsigned offset, unsigned len)
    {
        Up8 rd = _a.u;
        rd.c[0] -= uint8_t(offset);

        if(offset == 1)
        {
            emitMultiByte(*rd.p, len);
            return;
        }

        // Need to use explicit wraparound when reading window
        FGLCD_DUFF8(unsigned, len, {
            _a(*rd.p);
            ++rd.c[0];
        });
    }
};


template<PackType PK> struct NeedsDecompWindow
{
    static_assert(!(PackTypeProps[PK] & PP_COMPOUND), "eh?");
    static const bool value = !!(PackTypeProps[PK] & PP_HAS_WINDOW);
};

template<DecompDst, unsigned> struct _SelectWindowType;

template<unsigned WSZ> struct _SelectWindowType<ToRAM, WSZ>
{
    static const DecompWindowType value = DECOMP_WINDOW_OUTPUT;
};
template<unsigned WSZ> struct _SelectWindowType<ToLCD, WSZ>
{
    static const DecompWindowType value = (WSZ == 0)
        ? DECOMP_WINDOW_NONE
        : (WSZ <= 255 ? DECOMP_WINDOW_TINY : DECOMP_WINDOW_BIG);
};

template<DecompDst DST, PackType PK, unsigned WSZ>
struct SelectWindowType
{
    static const bool needwin = DST == ToRAM || (WSZ && NeedsDecompWindow<PK>::value);
    static const DecompWindowType wintype = needwin ? _SelectWindowType<DST, WSZ>::value : DECOMP_WINDOW_NONE;

    typedef typename SelectSideEffect<DST>::type SideEffect;

    typedef DecompEmitterWindow<wintype, SideEffect> type;
};
