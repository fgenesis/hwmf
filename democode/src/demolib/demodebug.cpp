#include "fglcd/lut_7seg.h"

#include "demodebug.h"
#include "debugthing.h"
#include "globals.h"
#include "scratch.h"

#include <stdio.h>
#include <string.h>


typedef fglcd::Pin<fglcd::PortB, 7> LED; // for debugging

#ifdef CFG_USE_DEBUG_THING

extern "C" const void *__bss_end;
extern "C" const void *__bss_start;
extern "C" const void *__heap_start;
extern "C" const void *__data_start;


void dbg_demomain(DebugThing& dbg, void *ud, uint8_t b, uint8_t bx)
{
    DebugVars& v = *((DebugVars*)ud);
    if(b & bx & 0x80)
        G.demo.partdone = true;
    if(b & bx & 0x40)
        ++v.what;
    if(DebugThing::Alone())
    {
        switch(v.what)
        {
            default:
                v.what = 0;
                /* fall through */
            case 0: case 1:
            {
                // RAMSTART <= __bss_end <= SP <= RAMEND
#ifdef __AVR__
                const int16_t ramstart = RAMSTART;
                //const int16_t heapstart = uintptr_t(__data_start); // this was __malloc_heap_start which should be the same, but pulls in some malloc() symbols that use RAM
                const int16_t heapstart = ramstart + sizeof(AllGlobals); // HACK
                const int16_t ramend = RAMEND;
                const int16_t sp = SP;
#else
                const int16_t ramstart = 0;
                const int16_t heapstart = 0;
                const int16_t ramend = 0x2000;
                const int16_t sp = 0x100;
#endif
                const int16_t staticused = heapstart - ramstart;
                const int16_t stackfree = sp - heapstart; // stack pointer grows downwards
                const int16_t stackused = ramend - sp;
#ifdef RAMPZ
                const uint8_t rampz = RAMPZ;
#else
                const uint8_t rampz = 0;
#endif
                if(v.what == 0)
                    dbg.set2Num(stackused, stackfree);
                else
                    dbg.set2Num(staticused, rampz);
                const uint8_t nev = evs::getNumEvents();
                uint8_t led = 0;
                for(uint8_t i = 0; i < nev; ++i)
                {
                    led <<= 1u;
                    led |= 1u;
                }
                dbg.setLEDs(led);
            }
            break;
#ifdef CFG_ENABLE_AUDIO
            case 2:
            {
                uint8_t haseff = 0;
                uint8_t vis[8];
                memset(vis, 0, 8);
                uint8_t on = 0;
                for(uint8_t i = 0; i < pmfplayer_max_channels; ++i)
                {
                    if(i < sizeof(vis))
                    {
                        const pmf_channel_info ch = G.mus.player.channel_info(i);
                        on <<= 1u;
                        if(ch.note_hit != v.chHit[i])
                        {
                            v.chHit[i] = ch.note_hit;
                            on |= 1;
                        }
                        haseff <<= 1u;
                        if(ch.effect != 0xff)
                            haseff |= 1u;
                        if(!(ch.base_note & 0x80))
                            vis[i] = ch.base_note & (~fglcd::seg7::DOT);
                    }
                }
                dbg.setLEDs(ulo8(on));
                for(uint8_t i = 0; i < 8; ++i, haseff >>= 1u)
                {
                    uint8_t x = vis[i];
                    if(haseff & 1)
                        x |= fglcd::seg7::DOT;
                    dbg.setRaw(i, x);
                }
            }
            break;

            case 3:
            {
                dbg.set2Num(G.mus.player.pattern_row(), G.mus.player.playlist_pos());
            }
            break;
#endif
        }
    }

    dbg.commit();

#if defined(CFG_ENABLE_AUDIO) && defined(MCU_IS_PC)
    //printf("%d / %d\n", player.pattern_row(), player.playlist_pos());
#endif
}


DebugVars::DebugVars()
{
    fglcd::RAM::Memset(this, 0, sizeof(*this));
}

#endif // defined CFG_USE_DEBUG_THING

#ifdef __AVR__

void _assert_fail(const char *cond_P, const char *msg_P, unsigned line, int val, int val2)
{
    cli();
    LED::makeOutput();

#ifdef CFG_USE_DEBUG_THING
    for(uint8_t i = 0; i < 2; ++i)
    {
        G.debug.device.setLEDs(0xff);
        fglcd::delay_ms(100);
        G.debug.device.setLEDs(0);
        fglcd::delay_ms(50);
    }
#endif

#ifdef CFG_USE_DEBUG_THING
    char mmsg[9];
    const char *numstr_P = PSTR("%8d");

    strncpy_P(mmsg, msg_P, 8);
    mmsg[8] = 0;
#endif
    for(;;)
    {
        LED::lo();
#ifdef CFG_USE_DEBUG_THING
        G.debug.device.setNum(line);
        G.debug.device.setLEDs(0x0c);
#endif
        fglcd::delay_ms(2000);

        LED::hi();
#ifdef CFG_USE_DEBUG_THING
        G.debug.device.setTextRAM(mmsg);
        G.debug.device.setLEDs(0x30);
#endif
        fglcd::delay_ms(2000);
        LED::lo();
#ifdef CFG_USE_DEBUG_THING
        G.debug.device.setNum(val);
        G.debug.device.setLEDs(val == val2 ? 0xc0 : 0x40);
#endif
        fglcd::delay_ms(2000);
#ifdef CFG_USE_DEBUG_THING
        if(val != val2)
        {
            G.debug.device.setNum(val2);
            G.debug.device.setLEDs(0x80);
            fglcd::delay_ms(2000);
        }
#endif
    }
}
#elif defined(MCU_IS_PC) // __AVR__
void _assert_fail(const char *cond_P, const char *msg_P, unsigned line, int val, int val2)
{
    fprintf(stderr, "ASSERTION FAILED [%u] <%s>: %s\n[val = %d], [val2 = %d]\n", line, msg_P, cond_P, val, val2);
    debug_break();
    abort();
}
#endif



void demo_init_debug()
{
#ifdef MCU_IS_PC
    printf("scratch0 = %p\n", scratch0);
#endif
    FGLCD_ASSERT((uintptr_t(scratch0) & 0xff) == 0, "scratcha");

#ifdef CFG_USE_DEBUG_THING
    DebugThing::StaticInit();
    G.debug.device.init();
#endif
}
