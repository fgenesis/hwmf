// Variants of libc functions that are much faster than those in avr-libc
// Not standards compliant (returns void instead of void*),
// so enable them only on when not on the PC, as they may conflict with system headers.

#pragma once

#include "mcu.h"

#ifndef MCU_IS_PC

#ifdef memset
#undef memset
#endif
#define memset(dst, b, n) fglcd::RAM::Memset((dst), (b), (n))

#ifdef memcpy
#undef memcpy
#endif
#define memcpy(dst, src, n) fglcd::RAM::Memcpy((void*)(dst), (const void*)(src), (n))

#ifdef memcpy_P
#undef memcpy_P
#endif
#define memcpy_P(dst, src, n) fglcd::Progmem::Memcpy((void*)(dst), (const void*)(src), (n))

#endif
