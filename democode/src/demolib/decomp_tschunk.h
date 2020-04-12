#pragma once

#include "decomp_util.h"
#include "decomp_rle.h"

namespace tschunk {

template<typename Mem, typename Emitter>
NOINLINE static void _decompress2(Emitter& em, typename Mem::Pointer p, unsigned ncmd)
{
    do
    {
        const uint8_t ctrl = Mem::readIncSafe(p);
        uint16_t n = decomp::readNRaw<Mem, 5>(ctrl, p);
        const uint8_t cmd = ctrl & 0xc0;
        if(!cmd) // 00: rep lit byte (n)
        {
            const uint8_t val = Mem::readIncSafe(p);
            em.emitMultiByte(val, n);
        }
        else if(cmd == 0x40) // 01: copy lit bytes (n)
        {
            if(n) // copy next n bytes
            {
                em.template emitLiterals<Mem>(p, n); 
            }
            else // skip next n bytes
            {
                n = decomp::readN<Mem>(p);
            }
            p += n;
        }
        else
        {
            uint8_t rep = 1;
            if(n & 1)
                rep += Mem::readIncSafe(p); // += 0xff wraps to 0x00, which is 0xff+1 times effectively

            n >>= 1u;
            typename Mem::Pointer pin = p;
            pin -= decomp::readN<Mem>(p); // offset backwards

            if(cmd == 0x80) // 10: copy pgm - offs (n)
            {
                do // always once, but rep more
                    em.template emitLiterals<Mem>(pin, n);
                while(--rep);
            }
            else     // 11: exec pgm - offs (n)
            {
                typename Mem::SegmentBackup seg;
                FGLCD_ASSERT(n, "tscexec0");
                /*if(!n) // special case: use strided-RLE variant
                {
                    n = decomp::readN<Mem>(p);
                    srledecomp_inner<Emitter, Mem>(em, p, pin, n, rep); // use rep as stride (default case of 1 is very fast)
                }
                else // recursively call into self at offset
                */    do
                        _decompress2<Mem, Emitter>(em, pin, n);
                    while(--rep);
            }
        }
    }
    while(--ncmd);
}

} // end namespace tschunk

template<typename Mem, typename Emitter>
FORCEINLINE void tschunkdecomp(Emitter& em, typename Mem::Pointer p, uint16_t ncmd)
{
    if(ncmd)
    {
        typename Mem::SegmentBackup seg;
        tschunk::_decompress2<Mem, Emitter>(em, p, ncmd);
    }
}
