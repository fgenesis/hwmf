#pragma once

// Helper to sync a variable with the music
// Intention: Whenever a note hits, set the value to peak,
// then (slowly) decay back to base.

#include "interpolator.h"
#include "musicsync.h"

template<typename T>
class MusicSyncVar : public interp::FancyInterpolatedValue<T>, public musync::Override
{
    typedef MusicSyncVar<T> Self;
public:
    T base;
    T peak;
    uint16_t speed;

    typedef interp::FancyInterpolatedValue<T> IVal;
    MusicSyncVar(interp::InterpolatorBase& inp, uint8_t channel, T base_, T peak_, uint16_t speed_)
        : IVal(base_)
        , musync::Override(channel, s_hit, this, musync::CallOnHit)
        , base(base_), peak(peak_), speed(speed_)
    {
        inp.add(*this);
    }

    void hit()
    {
        this->set(peak);
        this->interpolateTo(base, this->speed);
    }

private:
    static void s_hit(uint8_t channel, pmf_channel_info info, void *ud)
    {
        ((Self*)ud)->hit();
    }
};
