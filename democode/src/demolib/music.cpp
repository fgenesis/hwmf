#include "music.h"
#include "demo_def.h"
#include "globals.h"
#include "pmf_player.h"

#ifdef CFG_ENABLE_AUDIO

#ifdef CFG_MUSIC_SYNC_CHANNELS
static void TickCallback(void *ud)
{
    musync::UpdateCallbacks();
}
#endif

void music_update()
{
#ifndef MCU_IS_PC
    G.mus.player.update();
#endif
}

void music_stop()
{
    G.mus.player.stop();
}


bool music_init(const void *data)
{
    if(!G.mus.player.load(data))
        return false;

#ifdef CFG_MUSIC_SYNC_CHANNELS
    G.mus.player.set_tick_callback(TickCallback, NULL);
#endif
    return true;
}

void music_play(bool bg)
{
    G.mus.audiobuf.reset();
    G.mus.player.start(CFG_PMFPLAYER_SAMPLING_RATE);

    if(bg)
    {
        // FIXME: use timer template on AVR things
#ifdef MCU_IS_PC
        // nothing to do
#elif defined(ARDUINO_AVR_MEGA2560)
        // Interrupt music updater, uses timer 4
        TCCR4A=0;
        TCCR4B=_BV(CS40)|_BV(CS42)|_BV(WGM42); // CTC, /1024
        TCCR4C=0;
        TIMSK4=_BV(OCIE4A);
        OCR4A=0x10;
#elif defined(ARDUINO_AVR_UNO)
        // FIXME: THIS IS PROLLY TOTALLY WRONG
        /*TCCR2A=0;
        TCCR2B=_BV(CS20)|_BV(CS22)|_BV(WGM22); // CTC, /1024
        TCCR2C=0;
        TIMSK2=_BV(OCIE2A);
        OCR2A=0x10;*/
#else
#error dood
#endif
    }
}


#ifdef __AVR__
#ifdef ARDUINO_AVR_MEGA2560
ISR(TIMER4_COMPA_vect, ISR_NOBLOCK) // no blocking -> make sure music keeps playing while updating
#elif ARDUINO_AVR_UNO
ISR(TIMER2_COMPA_vect, ISR_NOBLOCK)
#else
#error dood
#endif
{
    G.mus.player.update();
}
#endif

#endif
