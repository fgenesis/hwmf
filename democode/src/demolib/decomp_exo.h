/*
 * Copyright (c) 2005 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented * you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 */

// Hacked to support Exomizer 3.0 and C++-ified -- fg
// For mode -P15

#pragma once

#include "decomp_window.h"
#include "demo_def.h"
#include "assert.h"

struct ExoState
{
    uint16_t base[52];
    uint8_t bits[52];
};

template<unsigned bits> struct TypeForBits { typedef typename TypeForBits<bits+1>::type type; };
template<> struct TypeForBits<8> { typedef uint8_t type; };
template<> struct TypeForBits<16> { typedef uint16_t type; };

// For some reason this decompresses garbage only
// But no time to fix it, i can see the deadline incoming aaaaargh
//#define READBYTE(x) (Reader::readIncSafe(x))

// This works...
#define READBYTE(x) (Reader::template read<uint8_t>(x++))

static FORCEINLINE uint8_t
bitbuffer_rotate(uint8_t *bb, uint8_t carry) // ROL
{
    const uint8_t carry_out = *bb >> 7u;
    *bb <<= 1;
    *bb |= carry;
    return carry_out;
}


template<typename Reader>
static FORCEINLINE uint16_t
read_bits(uint8_t *bit_buffer, typename Reader::Pointer& inp, uint8_t bit_count)
{
    uint16_t bits = 0;
    const uint8_t byte_copy = bit_count & 8;
    bit_count &= 7;
    
    uint8_t bb = *bit_buffer;
    while(bit_count--)
    {
        uint8_t carry = bitbuffer_rotate(&bb, 0);
        if (bb == 0)
        {
            //FGLCD_ASSERT(carry == 1);
            bb = READBYTE(inp);
            carry = bitbuffer_rotate(&bb, 1);
        }
        bits <<= 1;
        bits |= carry;
    }
    *bit_buffer = bb;

    if(byte_copy)
    {
        bits <<= 8;
        bits |= READBYTE(inp);
    }
    return bits;
}

template<typename Reader, uint8_t bit_count>
static FORCEINLINE typename TypeForBits<bit_count>::type
read_bits(uint8_t *bit_buffer, typename Reader::Pointer& inp)
{
    typedef typename TypeForBits<bit_count>::type R;
    return (R) read_bits<Reader>(bit_buffer, inp, bit_count);
}

// optional; for speed
template<typename Reader>
static NOINLINE uint8_t
read_bits(uint8_t *bit_buffer, typename Reader::Pointer& inp)
{
    uint8_t bb = *bit_buffer;
    uint8_t carry = bitbuffer_rotate(bb, 0);
    if (bb == 0)
    {
        assert(carry == 1);
        bb = READBYTE(inp);
        carry = bitbuffer_rotate(bb, 1);
    }
    *bit_buffer = bb;
    return carry;
}




template<typename Reader>
static void
init_table(ExoState& exo, typename Reader::Pointer& inp)
{
    uint8_t bit_buffer = 0x80;
    uint16_t b2;
    for(uint8_t i = 0; i < 52; ++i)
    {
        if((i & 15) == 0)
            b2 = 1;
        exo.base[i] = b2;

        uint8_t b1 = read_bits<Reader, 3>(&bit_buffer, inp);
        b1 |= read_bits<Reader, 1>(&bit_buffer, inp) << 3;
        exo.bits[i] = b1;

        b2 += 1u << b1;
    }
}

template<typename Reader>
static NOINLINE uint16_t
read_length(const ExoState& exo, uint8_t *bit_buffer, typename Reader::Pointer& in, uint8_t index)
{
    return exo.base[index] + read_bits<Reader>(bit_buffer, in, exo.bits[index]);
}

template<typename Reader>
static NOINLINE uint8_t
read_gamma(uint8_t *bit_buffer, typename Reader::Pointer& in)
{
    uint8_t g = 0;
    while(!read_bits<Reader, 1>(bit_buffer, in))
        ++g;
    return g;
}

template<typename Reader>
typename Reader::Pointer
exo_decrunch_init(ExoState& exo, typename Reader::Pointer in)
{
    typename Reader::SegmentBackup seg;
    init_table<Reader>(exo, in);
    return in;
}

template<typename Reader, typename Emit>
void
exo_decrunch(const ExoState& exo, typename Reader::Pointer in, Emit& out)
{
    typename Reader::SegmentBackup seg;
    uint8_t bit_buffer = 0x80;
    goto implicit_literal_byte;

    for(;;)
    {
        if(read_bits<Reader, 1>(&bit_buffer, in)) // literal?
        {
            implicit_literal_byte:
            const uint8_t c = READBYTE(in);
            out.emitSingleByte(c);
            continue;
        }
        uint8_t index = read_gamma<Reader>(&bit_buffer, in);
        if(index == 16) // stop code?
            break;
        if(index == 17)
        {
            const uint8_t hi = READBYTE(in);
            const uint8_t lo = READBYTE(in);
            const uint16_t length = (hi << 8u) | lo;
            out.template emitLiterals<Reader>(in, length); // TODO: make fast yo
            continue;
        }
        const uint16_t length = read_length<Reader>(exo, &bit_buffer, in, index);
        
        index = (length < 3 ? length : 3) - 1u;
        index = (48u - 16u*index) + read_bits<Reader>(&bit_buffer, in, index ? 4 : 2);

        const uint16_t offset = read_length<Reader>(exo, &bit_buffer, in, index);
        out.emitOffset(offset, length);
    }
}

template<typename Reader, typename Emit>
void
exo_decrunch_single(typename Reader::Pointer in, Emit& out)
{
    typename Reader::SegmentBackup seg;
    ExoState exo;
    in = exo_decrunch_init<Reader>(exo, in);
    exo_decrunch<Reader, Emit>(exo, in, out);
}

#undef READBYTE


template<typename Reader>
struct DecompState<PK_BLOCK_EXO, Reader>
{
    typename Reader::Pointer _data, _offsets;

    ExoState exo;

    DecompState(typename Reader::Pointer src, unsigned)
    {
        src = exo_decrunch_init<Reader>(exo, src);
        
        const uint16_t N = Reader::template read<uint16_t>(src); src += 2;
        const uint16_t skip = 2u * N;
        _offsets = src; src += skip;
        _data = src;
    }

    template<DecompDst dd, unsigned WSZ>
    void decompBlock(void *dst, unsigned block) const
    {
        FGLCD_ASSERT(_offsets + 2*block < _data, "exodcblk");
        typedef typename SelectWindowType<dd, PK_EXO, WSZ>::type Window;
        Window w(dst);
        const uint16_t offs = Reader::template read<uint16_t>(_offsets, block);
        const typename Reader::Pointer p = _data + offs;

        exo_decrunch<Reader, Window>(exo, p, w);
    }
};
