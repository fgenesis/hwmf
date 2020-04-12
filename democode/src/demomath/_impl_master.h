#pragma once

#include "../demolib/cfg-demo.h"

#ifdef FGLCD_ASM_AVR
#  define _IMPL_ARCH _AVR
#  include "_impl_avr.h"
#else
#  define _IMPL_ARCH _CPP
#  include "_impl_cpp.h"
#endif

#define _JOIN2(a,b) a##b
#define _JOIN(a,b) _JOIN2(a,b)

#define _IMPL(ty, fn, args, call) \
    static FORCEINLINE ty fn args \
    { return _JOIN(fn, _IMPL_ARCH) call; }
#include "_impl_list.h"

#undef _JOIN
#undef _JOIN2
