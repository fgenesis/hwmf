// RLE compressor that takes byte-wise input
// starts equal runs on the 3rd consecutive equal char
// output may be different from the RLE compressor in the tools (rle.h)
// but does not need the complete input up-front

// FIXME TEST THIS

#include "comp_rle.h"

enum RLEState
{
    UNDECIDED,
    COPY,
    FILL
};

enum RLEOpMask
{
    M_COPY = 0x80,
    M_FILL = 0
};

enum RLELimits
{
    RLEMAX = 127 // inclusive
};

RLECompState::RLECompState(void * dst)
    : wptr((uint8_t*)dst)
    , ctrlp(NULL)
    , offs(0)
    , lastbyte(0)
    , ctr(0)
    , base((uint8_t*)dst)
{
}

static FORCEINLINE void flushfill(RLECompState *rs, uint8_t b)
{
    // must write control byte and rep byte
    *rs->wptr++ = b;
    *rs->ctrlp = M_FILL | rs->ctr;
#ifdef _DEBUG
    rs->ctrlp = NULL;
#endif
}

static FORCEINLINE void flushcopy(RLECompState *rs, uint8_t len)
{
    // bytes were already written, but control byte must be put
    *rs->ctrlp = M_COPY | len;
#ifdef _DEBUG
    rs->ctrlp = NULL;
#endif
}

static FORCEINLINE uint8_t copylen(RLECompState *rs)
{
    return uint8_t(rs->wptr - rs->ctrlp) - 1;
}


void RLECompState::addByte(uint8_t b)
{
    switch(state)
    {
        case FILL:
            if(b == lastbyte && ctr < RLEMAX)
                ++ctr;
            else
            {
                flushfill(this, b);
                goto startfresh;
            }
            break;

        case COPY: // in this mode, ctr counts repetitions (consecutive eq. bytes)
        {
            // can we fit another byte? if not, finish the run and restart
            const uint8_t len = copylen(this);
            if(len >= RLEMAX)
            {
                flushcopy(this, len);
                goto startfresh;
            }
            *wptr++ = b;
            if(b != lastbyte)
            {
                lastbyte = b;
                ctr = 0;
            }
            else
            {
                if(ctr == 2)
                {
                    // 3rd equal byte, finish copy run and start a fill run
                    wptr -= 2; // 2 equal bytes already written
                    flushcopy(this, len);
                    goto startfill;
                }
                ++ctr;
            }
        }
        break;

        case UNDECIDED:
            switch(ctr)
            {
            startfresh: // yeah i know. labels in switches, yolo
                state = UNDECIDED;
                ctr = 0;
            case 0: // first byte after a run, nothing to do at this point
                lastbyte = b;
                break;
            case 1: // 2nd byte, mismatch opens a copy run
                if(b != lastbyte)
                    goto mismatch;
                break; // nothing to do
            case 2: // 3rd byte in a sequence, another match opens a fill run
                if(b == lastbyte)
                {
                startfill:
                    state = FILL;
                    uint8_t *p = wptr;
                    ctrlp = p++;
                    // ctr is already 2 here
                }
                else // two equal bytes, but 3rd byte is different
                {
                    mismatch:
                    uint8_t *p = wptr;
                    ctrlp = p++; // will write the size to here
#ifdef _DEBUG
                    *ctrlp = 0xff;
#endif
                    // flush 1 or 2 buffered, equal bytes
                    {
                        uint8_t prev = lastbyte;
                        uint8_t todo = ctr; // 1 or 2, never 0
                        FGLCD_ASSERT(todo >= 1 && todo <= 2, "rleoops");
                        do
                            *p++ = prev;
                        while(--todo);
                    }

                    *p++ = b; // this is the mismatched byte
                    wptr = p;

                    ctr = 0;
                    state = COPY;
                }
                break;
            }
            ++ctr;
            break;

    }
}

void RLECompState::addByteRep(uint8_t b, uint16_t n)
{
    // first up to 3 bytes will bring it into the right state cleanly
    // must always push one byte even if we're already in FILL mode, since
    // the last char might be different from what we have now.
    do
    {
        if(!n)
            return;
        --n;
        addByte(b);
    }
    while(state != FILL);

    // now we know: state == FILL, lastbyte == b
    // unknown: ctr = ??

    // from here we can go in big blocks
    uint32_t N = uint32_t(n) + ctr; // maybe there's something in the pipeline already
    uint8_t *p = wptr, *c = ctrlp;
    while(N > RLEMAX) // emit full runs (2 bytes each)
    {
        *c = RLEMAX;
        *p++ = b;
        c = p++;
        N -= RLEMAX;
    }
    // remaining incomplete run
    // (if exactly RLEMAX, run will be completed on next append (or flush))
    wptr = p;
    ctrlp = c;
    ctr = N; // meaning is the same for both modes below
    state = N <= 2 ? UNDECIDED : FILL;
    //lastbyte = b; // already known
}

unsigned RLECompState::flush()
{
    switch(state)
    {
        case UNDECIDED: // got 1 byte or 2 equal bytes -> make this a fill run, too
            FGLCD_ASSERT(ctr < 2, "rlepflu");
            // fall through
        case FILL: // 3+ bytes
            flushfill(this, lastbyte);
            break;

        case COPY:
            flushcopy(this, copylen(this));
            break;
    }

    ctr = 0;
    state = UNDECIDED;

    const unsigned oldoffs = offs;
    *wptr++ = 0;
    offs = wptr - base;
    return oldoffs;
}
