#pragma once

#include "fp8_8.h"
#include "fp16_16.h"
#include "staticdata.h"


static  int8_t isin8slow(uint8_t x) { return fglcd::ProgmemFar::template read<uint8_t>(fglcd_get_farptr(lut_isintab), x); }
static uint8_t usin8slow(uint8_t x) { return fglcd::ProgmemFar::template read<uint8_t>(fglcd_get_farptr(lut_isintab), x) + uint8_t(127); }

static  int8_t icos8slow(uint8_t x) { return fglcd::ProgmemFar::template read<uint8_t>(fglcd_get_farptr(lut_isintab), uint8_t(x + 64)); }
static uint8_t ucos8slow(uint8_t x) { return fglcd::ProgmemFar::template read<uint8_t>(fglcd_get_farptr(lut_isintab), uint8_t(x + 64)) + uint8_t(127); }

fp88 tan88slow(uint8_t x);
fp88 invtan88slow(uint8_t x);
