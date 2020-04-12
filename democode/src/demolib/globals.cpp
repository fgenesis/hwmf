#include "globals.h"

#if defined(__AVR__) && (CFG_SCRATCH_ALIGNMENT == 256)
// The linker will place this at the lower end of RAM, which is aligned (linker map: 0x00800200)
// If it doesn't, something is either very fucked up or some idiot introduced a global somewhere.
#define SCRATCH_ALN
#else
#  if CFG_SCRATCH_ALIGNMENT+0
#    define SCRATCH_ALN ALIGN(CFG_SCRATCH_ALIGNMENT)
#  else
#    define SCRATCH_ALN
#  endif
#endif

SCRATCH_ALN AllGlobals G;
