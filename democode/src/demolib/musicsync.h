#pragma once

#include "demo_def.h"
#include "pmf_player_def.h"

namespace musync {

enum CallFilter
{
    CallOnHit = 0,
    CallAlways = 1
};

typedef void (*ChannelCallbackFunc)(uint8_t channel, pmf_channel_info info, void *ud);

struct ChannelCallback
{
    ChannelCallbackFunc func;
    void *ud;
    uint8_t filter; // CallFilter
};

void SetChannelCallback(uint8_t channel, ChannelCallbackFunc f, void *ud, CallFilter when);

void UpdateCallbacks();


// Helper to temporarily install a sync function
struct Override
{
    const uint8_t channel;

    FORCEINLINE Override(uint8_t channel, ChannelCallbackFunc f, void *ud, CallFilter when)
    : channel(channel)
    {
        SetChannelCallback(channel, f, ud, when);
    }

    FORCEINLINE ~Override()
    {
        SetChannelCallback(channel, NULL, NULL, CallOnHit);
    }
};

} // end namespace musync
