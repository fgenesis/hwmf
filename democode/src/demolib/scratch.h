#pragma once

#include <stdint.h>
#include "cfg-demo.h"
#include "globals.h"

// msvc you piece of fuck, why
#ifdef _MSC_VER

#if CFG_SCRATCH_MAX_AREAS+0
#define scratch0 (&G.scratchblock[0])
#endif
#if CFG_SCRATCH_MAX_AREAS > 1
#define scratch1 (&G.scratchblock[CFG_SCRATCH_BLOCK_SIZE])
#endif
#if CFG_SCRATCH_MAX_AREAS > 2
#define scratch2 (&G.scratchblock[2*CFG_SCRATCH_BLOCK_SIZE])
#endif
#if CFG_SCRATCH_MAX_AREAS > 3
#define scratch3 (&G.scratchblock[3*CFG_SCRATCH_BLOCK_SIZE])
#endif

#else

#if CFG_SCRATCH_MAX_AREAS+0
static constexpr uint8_t * const scratch0 = &G.scratchblock[0];
#if CFG_SCRATCH_MAX_AREAS > 1
static constexpr uint8_t * const scratch1 = &G.scratchblock[CFG_SCRATCH_BLOCK_SIZE];
#endif
#if CFG_SCRATCH_MAX_AREAS > 2
static constexpr uint8_t * const scratch2 = &G.scratchblock[2*CFG_SCRATCH_BLOCK_SIZE];
#endif
#if CFG_SCRATCH_MAX_AREAS > 3
static constexpr uint8_t * const scratch3 = &G.scratchblock[3*CFG_SCRATCH_BLOCK_SIZE];
#endif
#endif

#endif
