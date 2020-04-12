//============================================================================
// PMF Player
//
// Copyright (c) 2019, Profoundic Technologies, Inc.
// All rights reserved.
//----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Profoundic Technologies nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL PROFOUNDIC TECHNOLOGIES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

#ifndef PFC_PMF_PLAYER_H
#define PFC_PMF_PLAYER_H
//---------------------------------------------------------------------------


//============================================================================
// interface
//============================================================================
// external
#include "pmf_player_def.h"
#include <Arduino.h>

//===========================================================================
// logging
//===========================================================================
#if PMF_USE_SERIAL_LOGS==1
#define PMF_SERIAL_LOG(...) {char buf[64]; sprintf(buf, __VA_ARGS__); Serial.print(buf);}
#else
#define PMF_SERIAL_LOG(...)
#endif
//---------------------------------------------------------------------------


template<typename T, bool stereo, unsigned channel_bits>
void pmf_player::mix_buffer_impl(pmf_mixer_buffer &buf_, unsigned num_samples_)
{
  T * const bufstart=(T*)buf_.begin, * const buffer_end=bufstart+num_samples_*(stereo?2:1);
  memset(bufstart, 0, (buffer_end - bufstart) * sizeof(T));
  audio_channel *channel=m_channels, *channel_end=channel+m_num_playback_channels;
  do
  {
    // check for active channel
    if(!channel->sample_speed)
      continue;

    // get channel attributes
    size_t sample_addr=(size_t)(m_pmf_file+pgm_read_dword(channel->smp_metadata+pmfcfg_offset_smp_data));
    uint32_t sample_pos=channel->sample_pos;
    int16_t sample_speed=channel->sample_speed;
    uint32_t sample_end=uint32_t(pgm_read_dword(channel->smp_metadata+pmfcfg_offset_smp_length))<<8;
    uint32_t sample_loop_len=(pgm_read_dword(channel->smp_metadata+pmfcfg_offset_smp_loop_length_and_panning)&0xffffff)<<8;
    uint8_t sample_volume=(channel->sample_volume*(channel->vol_env.value>>8))>>8;
    uint32_t sample_pos_offs=sample_end-sample_loop_len;
    if(sample_pos<sample_pos_offs)
      sample_pos_offs=0;
    sample_addr+=sample_pos_offs>>8;
    sample_pos-=sample_pos_offs;
    sample_end-=sample_pos_offs;

    // setup panning
    int8_t panning=channel->sample_panning;
    int16_t sample_phase_shift=panning==-128?0xffff:0;
    panning&=~int8_t(sample_phase_shift);
    uint8_t sample_volume_l=uint8_t((uint16_t(sample_volume)*uint8_t(128-panning))>>8);
    uint8_t sample_volume_r=uint8_t((uint16_t(sample_volume)*uint8_t(128+panning))>>8);

    // mix channel to the buffer
    T *buf=bufstart;
    do
    {
      // get sample data and adjust volume
#if PMF_USE_LINEAR_INTERPOLATION==1
      uint16_t smp_data=((uint16_t)pgm_read_word(sample_addr+(sample_pos>>8)));
      uint8_t sample_pos_frc=sample_pos&255;
      int16_t smp=((int16_t(int8_t(smp_data&255))*(256-sample_pos_frc))>>8)+((int16_t(int8_t(smp_data>>8))*sample_pos_frc)>>8);
#else
      int16_t smp=(int8_t)pgm_read_byte(sample_addr+(sample_pos>>8));
#endif

      // mix the result to the audio buffer (the if-branch with compile-time constant will be optimized out)
      if(stereo)
      {
        (*buf++)+=T(sample_volume_l*smp)>>(16-channel_bits);
        (*buf++)+=T(sample_volume_r*(smp^sample_phase_shift))>>(16-channel_bits);
      }
      else
        (*buf++)+=T(sample_volume*smp)>>(16-channel_bits);

      // advance sample position
      sample_pos+=sample_speed;
      if(sample_pos>=sample_end)
      {
        // check for loop
        if(!sample_loop_len)
        {
          channel->sample_speed=0;
          break;
        }

        // apply normal/bidi loop
        if(pgm_read_byte(channel->smp_metadata+pmfcfg_offset_smp_flags)&pmfsmpflag_bidi_loop)
        {
          sample_pos-=sample_speed*2;
          channel->sample_speed=sample_speed=-sample_speed;
        }
        else
          sample_pos-=sample_loop_len;
      }
    } while(buf<buffer_end);
    channel->sample_pos=sample_pos+sample_pos_offs;
  } while(++channel!=channel_end);

  // advance buffer
  ((T*&)buf_.begin)+=num_samples_*(stereo?2:1);
  buf_.num_samples-=num_samples_;
}
//---------------------------------------------------------------------------

template<typename T, unsigned buffer_size>
void pmf_audio_buffer<T, buffer_size>::reset()
{
  playback_pos=0;
  subbuf_write_idx=1;
  memset(buffer, 0, sizeof(buffer));
}
//----

template<typename T, unsigned buffer_size>
template<typename U, unsigned sample_bits, unsigned preshift>
inline U pmf_audio_buffer<T, buffer_size>::read_sample()
{
  // read sample from the buffer and clip to given number of bits
  enum {sample_range=1<<sample_bits, max_sample_val=sample_range-1, halfrange=sample_range>>1};
  uint16_t pbpos=playback_pos;
  T rawsmp = buffer[pbpos] >> preshift;
  U smp=U(rawsmp+halfrange);
  if(smp>sample_range-1)
    smp=smp>((U(-1)>>1)+halfrange)?0:max_sample_val;
  if(++pbpos==buffer_size)
    pbpos=0;
  playback_pos=pbpos;
  return smp;
}
//----

template<typename T, unsigned buffer_size>
pmf_mixer_buffer pmf_audio_buffer<T, buffer_size>::get_mixer_buffer()
{
  // return buffer for mixing if available (i.e. not playing the one for writing)
  uint16_t pbpos=playback_pos; // note: atomic read thus no need to disable interrupts
  pmf_mixer_buffer buf={0, 0};
  if(subbuf_write_idx^(pbpos<subbuf_size))
    return buf;
  buf.begin=buffer+subbuf_write_idx*subbuf_size;
  buf.num_samples=subbuf_size;
  subbuf_write_idx^=1;
  return buf;
}
//---------------------------------------------------------------------------

//============================================================================
#endif
