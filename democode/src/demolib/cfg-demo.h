#pragma once

#include "fglcd/fglcd.h"
#include "fglcd/chip_tm1638.h"


typedef fglcd::Pin<fglcd::PortE, 3> MegaPin5;
typedef fglcd::Pin<fglcd::PortH, 3> MegaPin6;
typedef fglcd::Pin<fglcd::PortH, 4> MegaPin7;
typedef fglcd::TM1638<MegaPin5, MegaPin6, MegaPin7> Panel;

typedef fglcd::preset::ILI9481_Mega_Shield LCD;

typedef LCD::ColorType Color;

#define CFG_MAXRAM 8192 // in bytes
#define CFG_RAM_SAFETY_AREA 256 // in bytes. Attempt to keep this much RAM free for interrupts, stack, etc.

#define CFG_SCRATCH_MAX_AREAS 4  // 0/undef to disable scratch
#define CFG_SCRATCH_BLOCK_SIZE 256
#define CFG_SCRATCH_ALIGNMENT 256

#define CFG_PAL_SIZE 256
#define CFG_PAL_START_LO (scratch0)
#define CFG_PAL_START_HI (scratch1) // must be directly located behind CFG_PAL_START_LO in memory

#define CFG_PTR_ISINTAB (scratch2)
#define CFG_PTR_USINTAB (scratch3)

#define CFG_PTR_DECOMPWINDOW (scratch3)

#define CFG_MAX_EVENTS 8 // 0/undef to disable event system
#define CFG_USE_HIGH_PRECISION_EVENT_TIMER 0

#define CFG_ENABLE_AUDIO
#define CFG_MUSIC_SYNC_CHANNELS 4 // undef to disable music sync
#define CFG_PMFPLAYER_AUDIO_BUFFER_SIZE 256
#define CFG_PMFPLAYER_SAMPLING_RATE 13000

#define CFG_FAST_TRIANGLE_SORTING 1 // 0/undef to disable; fast sorting needs alloca() and some extra bytes
//#define CFG_FLOAT_DEPTH 0 // 0/undef to disable; only for debugging!

#ifdef FGLCD_DEBUG
#define CFG_USE_DEBUG_THING
#endif


/*
typedef fglcd::preset::ILI9486_Uno_Shield LCD;

typedef fglcd::Pin<fglcd::PortB, 3> UnoPin17;
typedef fglcd::Pin<fglcd::PortB, 4> UnoPin18;
typedef fglcd::Pin<fglcd::PortB, 5> UnoPin19;
typedef fglcd::TM1638<UnoPin17, UnoPin18, UnoPin19> Panel;

#define CFG_SCRATCH_MAX_AREAS 2  // 0/undef to disable scratch
#define CFG_SCRATCH_BLOCK_SIZE 256
#define CFG_SCRATCH_ALIGNMENT 256

#define CFG_PAL_SIZE 128
#define CFG_PAL_START_LO (scratch0)
#define CFG_PAL_START_HI (&scratch0[128]) // must be directly located behind CFG_PAL_START_LO in memory

#define CFG_PTR_ISINTAB (scratch1)
#define CFG_PTR_USINTAB (scratch1)

#define CFG_PTR_DECOMPWINDOW (scratch1)

#define CFG_MAX_EVENTS 4 // 0/undef to disable event system
#define CFG_USE_HIGH_PRECISION_EVENT_TIMER 0

*/

