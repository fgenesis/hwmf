#pragma once

#include "decomp_window.h"
#include "../demomath/fgmath.h"

template<typename Emit, typename Reader>
void rledecomp(Emit& out, typename Reader::Pointer src)
{
    typename Reader::SegmentBackup seg;

    while(unsigned char c = Reader::readIncSafe(src))
    {
        if(c & 0x80) // copy
        {
            c &= ~0x80;
            out.template emitLiterals<Reader>(src, c);
            src += c;
        }
        else // fill
        {
            const unsigned char v = Reader::readIncSafe(src);
            out.emitMultiByte(v, c);
        }
    }
}

template<typename Reader>
struct DecompState<PK_BLOCK_RLE, Reader>
{
    typename Reader::Pointer _data, _offsets;

    DecompState(typename Reader::Pointer src, unsigned)
    {
        const uint16_t N = Reader::template read<uint16_t>(src); src += 2;
        const uint16_t skip = 2u * N;
        _offsets = src; src += skip;
        _data = src;
    }

    template<DecompDst dd, unsigned WSZ>
    void decompBlock(void *dst, unsigned block) const
    {
        FGLCD_ASSERT(_offsets + 2*block < _data, "rledcblk");
        typedef typename SelectWindowType<dd, PK_RLE, WSZ>::type Window;
        Window w(dst);
        const uint16_t offs = Reader::template read<uint16_t>(_offsets, block);
        const typename Reader::Pointer p = _data + offs;

        rledecomp<Window, Reader>(w, p);
    }
};


// NOT YET USED
template<typename Emit, typename Reader>
FORCEINLINE void srledecomp_inner(Emit& out, typename Reader::Pointer& src, typename Reader::Pointer& valptr, unsigned num, uint8_t stride)
{
    typename Reader::SegmentBackup seg;

    // valptr and src are both located in the same memory type.
    // But we need to read from two distinct pointers. And the only pointer that can read from PROGMEM is Z.
    // Usually that would require changing Z twice per value:
    // The first time to read the value (single byte), the second time to read the count (another single byte)
    // In order to prevent Z ping-pong, copy the values data into scratch space so we can use X or Y for the values ptr.
    // This speeds up processing a lot since auto-incrementing reads can be used, and AVR's annoying 24-bit addressing is less of a hassle.
    uint8_t * const lut = CFG_PTR_DECOMPWINDOW;

    while(num)
    {
        // copy values LUT to RAM, packed
        const uint8_t n = (uint8_t)vmin<uint16_t>(num, 255);
        {
            if(stride == 1)
            {
                Reader::Memcpy(lut, valptr, n);
                valptr += n;
            }
            else // strided copy, can't autoinc anymore and need to use the slower copy code
            {
                uint8_t *dst = lut;
                uint8_t nn = n;
                do
                {
                    const uint8_t x = Reader::template read<uint8_t>(valptr);
                    fglcd::RAM::storeIncFast(dst, x);
                    valptr += stride;
                }
                while(--nn);
            }
        }

        // main copy loop
        //Reader::SetSegmentPtr(src);
        uint8_t nn = n;
        const uint8_t * lutrd = lut;
        do
        {
            const uint8_t val = fglcd::RAM::readIncSafe(lutrd); // could be replaced by {val = *valptr; valptr += stride;} with the above block omitted if this wasn't supposed to be fast on AVR
            if(uint16_t rep = decomp::readN<Reader>(src))
                out.emitMultiByte(val, rep);
        }
        while(--nn); // 256 times if begin with nn == 0
        //Reader::fixFarPtr(src);
        num -= n;
    }
}

template<typename Emit, typename Reader>
void srledecomp(Emit& out, typename Reader::Pointer src, unsigned offset, unsigned num, uint8_t stride)
{
    typename Reader::Pointer valptr = src - offset;
    if(!offset)
        src += num;
    srledecomp_inner<Emit, Reader>(out, src, valptr, num, stride);
}
