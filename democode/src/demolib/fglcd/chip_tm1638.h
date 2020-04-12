#pragma once

#include "mcu.h"

#ifdef MCU_IS_PC
#include "chip_tm1638_emu.h"
#else
#include "chip_tm1638_hw.h"
#endif
