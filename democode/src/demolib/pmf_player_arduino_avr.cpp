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


#ifdef __AVR__
#pragma GCC optimize ("O3")
#include "cfg-demo.h"

#ifdef CFG_ENABLE_AUDIO

#include "pmf_data.h"
#include "pmf_player.h"
//---------------------------------------------------------------------------

//===========================================================================
// audio buffer
//===========================================================================
#include "globals.h"
//---------------------------------------------------------------------------

//===========================================================================
// pmf_player
//===========================================================================

// This will use the GPIOR0, GPIOR1 & GPIOR2 I/O registers instead of the stack
// for backing up registers in the audio interrupt. Those registers are usually
// not used, certainly not by the compiler, so why not use them to speed up
// the interrupt. push/pop need 2 cycles, in/out only 1 cycle.
// Saves 6 cycles per sample.
#define USE_GPIO_REGS



// Main music player interrupt, called with high frequency
ISR(TIMER3_COMPA_vect, ISR_NAKED)
{
    // Compiler produces bad code for this
    //const uint8_t s = G. mus.audiobuf.read_sample<uint8_t, 8, 2>();


    // crazy optimized asm version
    // Uses only 3 regs: X (r26, r27) and 1 other reg
    // ONLY works because:
    // - G.mus.audiobuf.buffer is page-aligned (256 bytes) and power-of-2 size
    // - That buffer size is int16_t[256]
    //   (2 pages of uint8, basically)
    // Difference to C++ version:
    // - pre-increments the buf pos so we skip the very first sample

    uint8_t s; // sample value
    asm volatile(
        // preserve state
        "push %[s] \n\t"
        "in %[s], __SREG__\n\t"
#ifdef USE_GPIO_REGS
        "out %[T1], %[s]  \n\t"
        "out %[T2], r26   \n\t"
        "out %[T3], r27   \n\t"
#else
        "push %[s] \n\t"
        "push r26  \n\t"
        "push r27  \n\t"
#endif

        // -- main part start -- 
        "clr %[s]             \n\t" // need one zero later
        // load, inc, store offset
        "lds r26, %a[pp]      \n\t" // load pos into X
        "inc r26              \n\t" // increment offset with wrap-around (carry is unused)
        "sts %a[pp], r26      \n\t" // store back for next time
        // offset buffer ptr (we know that the low-byte of buf is 0, since it's 256b-aligned)
        "ldi r27, hi8(%a[buf])\n\t" // load high byte
        "lsl r26              \n\t" // prepare for 16bit addressing (buf += (2 * offset))
        "adc r27, %[s]        \n\t" // need the carry; s is known to be 0
        // X now points to the 16bit sample to read, load both halves
        "ld %[s], X+          \n\t" // low byte goes into output
        "ld r26, X            \n\t" // high byte goes into X, which isn't needed anymore
        // preshift sample
        "asr r26              \n\t" // 2x 16bit signed shift right
        "ror %[s]             \n\t"
        "asr r26              \n\t"
        "ror %[s]             \n\t"
        // now s is the int8 sample
        : [s] "=&d" (s) /*out*/
        :   [buf] "p" (&G.mus.audiobuf.buffer[0]) /*in*/
          , [pp] "p" (&G.mus.audiobuf.playback_pos)
#ifdef USE_GPIO_REGS
          , [T1] "I" (_SFR_IO_ADDR(GPIOR0))
          , [T2] "I" (_SFR_IO_ADDR(GPIOR1))
          , [T3] "I" (_SFR_IO_ADDR(GPIOR2))
#endif
        : "r26", "r27" /*clob*/
    );
    s += 127; // make unsigned

    OCR0A = s;
    OCR0B = s;

    // restore state and return
    asm volatile(
#ifdef USE_GPIO_REGS
        "in r27, %[T3]   \n\t"
        "in r26, %[T2]   \n\t"
        "in %[s], %[T1]  \n\t"
#else
        "pop r27   \n\t"
        "pop r26   \n\t"
        "pop %[s]  \n\t"
#endif
        "out __SREG__, %[s]\n\t"
        "pop %[s]  \n\t"
        "reti      \n\t"
        : /*out*/
        :   [s] "r" (s) /*in*/
#ifdef USE_GPIO_REGS
          , [T1] "I" (_SFR_IO_ADDR(GPIOR0))
          , [T2] "I" (_SFR_IO_ADDR(GPIOR1))
          , [T3] "I" (_SFR_IO_ADDR(GPIOR2))
#endif
   );

}

//----

uint32_t pmf_player::get_sampling_freq(uint32_t sampling_freq_) const
{
  return sampling_freq_;
}
//----

void pmf_player::start_playback(uint32_t sampling_freq_)
{
  DDRB |= _BV(7);
  DDRG |= _BV(5);

    // PWM
    TCCR0A = (1<<COM0B1) | (1<<COM0A1) | (1<<WGM01) | (1<<WGM00); // Fast PWM, non-inverting mode
    TCCR0B = (1<<CS00);
    OCR0A = 0;
    OCR0B = 0;
    TCNT0 = 0;

  TCCR3A=0;
  TCCR3B=_BV(CS30)|_BV(WGM32); // CTC mode 4 (OCR1A)
  TCCR3C=0;
  TIMSK3=_BV(OCIE3A);          // enable timer 1 counter A
  OCR3A=(F_CPU+sampling_freq_/2)/sampling_freq_;
}
//----

void pmf_player::stop_playback()
{
  TIMSK3=0;
}
//----

void pmf_player::mix_buffer(pmf_mixer_buffer &buf_, unsigned num_samples_)
{
//  mix_buffer_impl(buf_, num_samples_);
  int16_t *buffer_begin=(int16_t*)buf_.begin, *buffer_end=buffer_begin+num_samples_;
  fglcd::RAM::Memset(buffer_begin, 0, num_samples_ * sizeof(*buffer_begin));
  audio_channel *channel=m_channels, *channel_end=channel+m_num_playback_channels;
  do
  {
    // check for active channel
    if(!channel->sample_speed)
      continue;

    // get channel attributes
    size_t sample_addr=(size_t)(m_pmf_file+pgm_read_dword(channel->smp_metadata+pmfcfg_offset_smp_data));
    uint16_t sample_len=pgm_read_word(channel->smp_metadata+pmfcfg_offset_smp_length);/*todo: should be dword*/
    uint16_t loop_len=pgm_read_word(channel->smp_metadata+pmfcfg_offset_smp_loop_length_and_panning);/*todo: should be dword*/
    uint8_t volume=(uint16_t(channel->sample_volume)*(channel->vol_env.value>>9))>>8;
    register uint8_t sample_pos_frc=channel->sample_pos;
    register uint16_t sample_pos_int=sample_addr+(channel->sample_pos>>8);
    register uint16_t sample_speed=channel->sample_speed;
    register uint16_t sample_end=sample_addr+sample_len;
    register uint16_t sample_loop_len=loop_len;
    register uint8_t sample_volume=volume;
    register uint8_t zero=0, upper_tmp;

    asm volatile
    (
      "push %A[buffer_pos] \n\t"
      "push %B[buffer_pos] \n\t"

      "mix_samples_%=: \n\t"
      "lpm %[upper_tmp], %a[sample_pos_int] \n\t"
      "mulsu %[upper_tmp], %[sample_volume] \n\t"
      "mov %[upper_tmp], r1 \n\t"
      "lsl %[upper_tmp] \n\t"
      "sbc %[upper_tmp], %[upper_tmp] \n\t"
      "ld __tmp_reg__, %a[buffer_pos] \n\t"
      "add __tmp_reg__, r1 \n\t"
      "st %a[buffer_pos]+, __tmp_reg__ \n\t"
      "ld __tmp_reg__, %a[buffer_pos] \n\t"
      "adc __tmp_reg__, %[upper_tmp] \n\t"
      "st %a[buffer_pos]+, __tmp_reg__ \n\t"
      "add %[sample_pos_frc], %A[sample_speed] \n\t"
      "adc %A[sample_pos_int], %B[sample_speed] \n\t"
      "adc %B[sample_pos_int], %[zero] \n\t"
      "cp %A[sample_pos_int], %A[sample_end] \n\t"
      "cpc %B[sample_pos_int], %B[sample_end] \n\t"
      "brcc sample_end_%= \n\t"
      "next_sample_%=: \n\t"
      "cp %A[buffer_pos], %A[buffer_end] \n\t"
      "cpc %B[buffer_pos], %B[buffer_end] \n\t"
      "brne mix_samples_%= \n\t"
      "rjmp done_%= \n\t"

      "sample_end_%=: \n\t"
      /*todo: implement bidi loop support*/
      "sub %A[sample_pos_int], %A[sample_loop_len] \n\t"
      "sbc %B[sample_pos_int], %B[sample_loop_len] \n\t"
      "mov __tmp_reg__, %A[sample_loop_len] \n\t"
      "or __tmp_reg__, %B[sample_loop_len] \n\t"
      "brne next_sample_%= \n\t"
      "clr %A[sample_speed] \n\t"
      "clr %B[sample_speed] \n\t"

      "done_%=: \n\t"
      "clr r1 \n\t"
      "pop %B[buffer_pos] \n\t"
      "pop %A[buffer_pos] \n\t"

      :[sample_speed] "+l" (sample_speed)
      ,[sample_pos_frc] "+l" (sample_pos_frc)
      ,[sample_pos_int] "+z" (sample_pos_int)

      :[sample_end] "r" (sample_end)
      ,[sample_volume] "a" (sample_volume)
      ,[upper_tmp] "a" (upper_tmp)
      ,[zero] "r" (zero)
      ,[sample_loop_len] "l" (sample_loop_len)
      ,[buffer_pos] "e" (buffer_begin)
      ,[buffer_end] "l" (buffer_end)
    );

    // store values back to the channel data
    channel->sample_pos=(long(sample_pos_int-sample_addr)<<8)+sample_pos_frc;
    channel->sample_speed=sample_speed;
  } while(++channel!=channel_end);

  // advance buffer
  ((int16_t*&)buf_.begin)+=num_samples_;
  buf_.num_samples-=num_samples_;
}
//----

pmf_mixer_buffer pmf_player::get_mixer_buffer()
{
  return G.mus.audiobuf.get_mixer_buffer();
}
//---------------------------------------------------------------------------

//===========================================================================
#endif // ARDUINO_ARCH_AVR

#endif
