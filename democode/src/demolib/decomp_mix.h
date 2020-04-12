#pragma once

#include "decomp_window.h"

template<typename Reader>
struct DecompState<PK_BLOCK_MIX, Reader>
{
    typename Reader::Pointer _data, _offsets, _ncmd;
    ExoState _exo;
    DecompState(typename Reader::Pointer src, unsigned)
    {
        typename Reader::SegmentBackup seg;
        const uint8_t usedbits = Reader::readIncSafe(src);
        if(usedbits & (1 << PK_EXO))
            src = exo_decrunch_init<Reader>(_exo, src);
        if(usedbits & (1 << PK_TSCHUNK))
        {
            uint16_t N = Reader::template read<uint16_t>(src); src += 2;
            _ncmd = src;
            src += 2u * N;
        }
        uint16_t noffs = Reader::template read<uint16_t>(src); src += 2;
        _offsets = src;
        src += 2u * noffs;

        _data = src;
    }

    struct Hdr
    {
        uint8_t algo;
        uint16_t num;
    };

    FORCEINLINE Hdr gethdr(typename Reader::Pointer& p) const
    {
        uint8_t b = Reader::readIncSafe(p);
        const uint8_t large = b & 1;
        b >>= 1u;
        uint8_t n = 0;
        uint8_t bit;
        do
        {
            bit = b & 1;
            b >>= 1u;
            n += bit;
        }
        while(bit);

        Hdr h;
        h.algo = n;
        h.num = large ? fglcd::make16(Reader::readIncSafe(p), b) : b;
        return h;
    }

    template<DecompDst dd, unsigned WSZ>
    void decompBlock(void *dst, unsigned block) const
    {
        typename Reader::SegmentBackup seg;
        typename Reader::Pointer p = _data + Reader::template read<uint16_t>(_offsets, block);
        const Hdr h = gethdr(p);
        switch(h.algo)
        {
            case PK_UNCOMP:
            {
                typedef typename SelectWindowType<dd, PK_UNCOMP, WSZ>::type Window;
                Window w(dst);
                w.template emitLiterals<Reader>(p, h.num);
                break;
            }
            case PK_RLE:
            {
                typedef typename SelectWindowType<dd, PK_RLE, WSZ>::type Window;
                Window w(dst);
                rledecomp<Window, Reader>(w, p);
                break;
            }
            case PK_EXO:
            {
                typedef typename SelectWindowType<dd, PK_EXO, WSZ>::type Window;
                Window w(dst);
                exo_decrunch<Reader, Window>(_exo, p, w);
                break;
            }
            case PK_TSCHUNK:
            {
                typedef typename SelectWindowType<dd, PK_TSCHUNK, WSZ>::type Window;
                Window w(dst);
                uint16_t ncmd = Reader::template read<uint16_t>(_ncmd, h.num);
                tschunkdecomp<Reader, Window>(w, p, ncmd);
                break;
            }
        }
    }
};


template<typename Reader>
struct DecompState<PK_BLOCK_TSCHUNK, Reader>
{
    typename Reader::Pointer _data, _offsets, _ncmd;
    uint8_t _big;
    DecompState(typename Reader::Pointer src, unsigned)
    {
        typename Reader::SegmentBackup seg;
        const uint16_t N = Reader::template read<uint16_t>(src); src += 2;
        const uint8_t big = Reader::readIncSafe(src);
        _big = big;
        const uint16_t skip = 2u * N;
        _ncmd = src; src += big ? skip : N;
        _offsets = src; src += skip;
        _data = src;
    }

    template<DecompDst dd, unsigned WSZ>
    void decompBlock(void *dst, unsigned block) const
    {
        typename Reader::SegmentBackup seg;
        FGLCD_ASSERT(_offsets + 2*block < _data, "tscdcblk");
        typedef typename SelectWindowType<dd, PK_TSCHUNK, WSZ>::type Window;
        Window w(dst);
        const uint16_t offs = Reader::template read<uint16_t>(_offsets, block);
        const uint16_t ncmd = _big ? Reader::template read<uint16_t>(_ncmd, block) : Reader::template read<uint8_t>(_ncmd, block);
        const typename Reader::Pointer p = _data + offs;
        tschunkdecomp<Reader, Window>(w, p, ncmd);
    }
};
