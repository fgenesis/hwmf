#include "demo.h"
#include "demoshared.h"
#include "src/demolib/demodebug.h"

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif

static void _doNextPart(void*)
{
    partdone = true;
}

void DoNextPartIn(unsigned ms)
{
    partdone = false;
    evs::schedule(ms, _doNextPart, NULL);
}

void WaitPartDone()
{
#ifdef MCU_IS_PC
    PalBackup bk;
    fuckpal(); // Visual indicator that we're waiting
#endif
    while(!partdone)
    {
#ifdef MCU_IS_PC
        SDL_Delay(1);
#endif
    }
    partdone = false;
}

void SyncChannel0Callback(uint8_t channel, pmf_channel_info info, void *ud)
{
    //printf("ch[%c]: note:%u, vol:%u, eff:%u, effd:%u\n",
    //    info.note_hit ? 'X' : ' ', info.base_note, info.volume, info.effect, info.effect_data);
    
    // TODO check note
    partdone = true;
}

void cls()
{
    LCD::clear(0);
}

static const uint8_t s_music[] PROGMEM = {
#include "music.gen.hh"
};

void demoinit()
{
    evs::init();
    demo_init_debug();

    LCD::init(); 
    LCD::enableCS();
    LCD::enableDisplay();

    fglcd::delay_ms(100); // Wait until shit has settled

    // If this ever triggers, check scratchbuffer alignment in src/demolib/globals.cpp
    u16 err = 0;
    if(uintptr_t(scratch0) & 0xff)
        err |= LCD::gencolor_inl(0xff,0,0);
    else if(uintptr_t(&G.mus.audiobuf.buffer[0]) & 0xff)
        err |= LCD::gencolor_inl(0,0xff,0);
    if(err)
    {
        LCD::clear(err);
        for(;;) {}
    }

    cls();
    part_thissideup();

    music_init(&s_music[0]);
    cls();
}

void demomain()
{
    musync::SetChannelCallback(0, SyncChannel0Callback, NULL, musync::CallOnHit);
    music_play(true);

    part_intro1(); evs::killAllEvents();
    cls();
    part_widescroll(); evs::killAllEvents();
    cls();
    part_intro2(); evs::killAllEvents();
    part_c64(); evs::killAllEvents();
    part_c64_to_cube(); evs::killAllEvents();
    cls();
    part_twist(); evs::killAllEvents();
    cls();
    part_wolf(); evs::killAllEvents();
    part_megadrivescroll(); evs::killAllEvents();
    part_test_landscape(); evs::killAllEvents();
    cls();
    part_greet(); evs::killAllEvents();
    part_gol(); evs::killAllEvents();
    cls();
    part_end();
}
