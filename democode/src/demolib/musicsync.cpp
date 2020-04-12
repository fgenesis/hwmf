#include "musicsync.h"
#include "globals.h"

namespace musync {

// Run from main thread
void SetChannelCallback(uint8_t channel, ChannelCallbackFunc f, void * ud, CallFilter when)
{
#ifdef CFG_MUSIC_SYNC_CHANNELS
    fglcd::NoInterrupt no;
    FGLCD_ASSERT(channel < MaxMusicSyncChannels, "musynccc");
    G.mus.sync[channel].func = f;
    G.mus.sync[channel].ud = ud;
#else
    FGLCD_ASSERT(0, "musynccc");
#endif
}

// Run from music update interrupt
void UpdateCallbacks()
{
#ifdef CFG_MUSIC_SYNC_CHANNELS
    for(uint8_t i = 0; i < MaxMusicSyncChannels; ++i)
    {
        if(ChannelCallbackFunc f = G.mus.sync[i].func)
        {
            const pmf_channel_info info = G.mus.player.channel_info(i);
            if(info.note_hit || G.mus.sync[i].filter)
                f(i, info, G.mus.sync[i].ud);
        }
    }
#endif
}

} // end namespace musync
