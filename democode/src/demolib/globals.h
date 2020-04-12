#pragma once

#include "cfg-demo.h"
#include "eventsystem.h"
#include "pmf_player_def.h"
#include "musicsync.h"

#ifdef FGLCD_DEBUG
#include "debugthing.h"
#include "demodebug.h"
#endif

#ifdef CFG_MUSIC_SYNC_CHANNELS
enum
{
    MaxMusicSyncChannels = ((0+CFG_MUSIC_SYNC_CHANNELS) < pmfplayer_max_channels)
        ? (0+CFG_MUSIC_SYNC_CHANNELS)
        : pmfplayer_max_channels
};
#endif

struct DemoGlobals
{
    volatile uint8_t partdone;
};

#ifdef FGLCD_DEBUG
struct DemoDebugGlobals
{
    // used by demolib
    DebugThing device;
    DebugVars vars;

    uint8_t numDebugInstances;

    // add some more?
};
#endif

#ifdef CFG_ENABLE_AUDIO
typedef pmf_audio_buffer<int16_t, CFG_PMFPLAYER_AUDIO_BUFFER_SIZE> AudioBufType;
struct DemoMusic
{
    AudioBufType audiobuf;
    pmf_player player;
#ifdef CFG_MUSIC_SYNC_CHANNELS
    musync::ChannelCallback sync[MaxMusicSyncChannels];
#endif
};
#endif

struct AllGlobals
{
    // IMPORTANT:
    // scratchblock *must* be aligned to a 256 byte boundary!
    // One way would be to align it right here, which is problematic because it forces the entire struct to be aligned,
    // which means its sizeof() will change.
    // Since this is a struct only to group the variables nicely and there's only once instance,
    // we align the struct singleton (see globals.cpp).
    // This way sizeof() is really just the size of the struct without alignment, effectively allowing to use a bit more RAM.
#if CFG_SCRATCH_MAX_AREAS+0
    uint8_t scratchblock[CFG_SCRATCH_MAX_AREAS * CFG_SCRATCH_BLOCK_SIZE];
#endif

    // mus.audiobuf.buffer *must* be aligned to a 256 byte boundary as well!
#ifdef CFG_ENABLE_AUDIO
    DemoMusic mus;
#endif

#if CFG_MAX_EVENTS+0
    evs::priv_Globals ev;
#endif

    DemoGlobals demo;

#ifdef FGLCD_DEBUG
    DemoDebugGlobals debug;
#endif
};


extern AllGlobals G;

#define MAXRAMFREE (CFG_MAXRAM - sizeof(AllGlobals) - CFG_RAM_SAFETY_AREA)
