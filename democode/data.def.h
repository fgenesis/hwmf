#pragma once
#include "src/demolib/demo_def.h"
#include "src/demolib/packdef.h"
#include "src/demomath/fp8_8.h"
#include "src/demomath/fp16_16.h"

#ifdef MCU_IS_PC
#  define PGM_ALIGN(x)
#else
#  define PGM_ALIGN(x) ALIGN(x)
#endif

