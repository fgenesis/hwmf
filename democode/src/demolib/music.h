#pragma once

#include "globals.h"

#ifdef CFG_ENABLE_AUDIO

FORCEINLINE static int16_t music_curSample16(uint8_t i)
{
    return G.mus.audiobuf.buffer[i];
}

bool music_init(const void *data);
void music_update();
void music_stop();
void music_play(bool bg);

#else

FORCEINLINE static bool music_init(bool bg, const void *data) { return false; }
FORCEINLINE static void music_update() {}
FORCEINLINE static void music_stop() {}

FORCEINLINE static int16_t music_curSample16(uint8_t i)
{
    return 0;
}

#endif
