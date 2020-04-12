#pragma once

#include "def.h"

#if defined(FGLCD_DEBUG)
#define FGLCD_ASSERT_VAL2(cond, msg, val, val2) do { if(!(cond)) { ::_assert_fail(PSTR(#cond), PSTR(msg), __LINE__, val, val2); } } while(0)
#define FGLCD_ASSERT_VAL(cond, msg, val) FGLCD_ASSERT_VAL2(cond, msg, val, val);
#define FGLCD_ASSERT(cond, msg) FGLCD_ASSERT_VAL(cond, msg, -1337);
#elif defined(_DEBUG)
#include <assert.h>
#define FGLCD_ASSERT(cond, msg) assert((cond) && msg)
#define FGLCD_ASSERT_VAL(cond, msg, val) assert((cond) && msg)
#define FGLCD_ASSERT_VAL2(cond, msg, val, val2) assert((cond) && msg)
#else
#define FGLCD_ASSERT(cond, msg)
#define FGLCD_ASSERT_VAL(cond, msg, val)
#define FGLCD_ASSERT_VAL2(cond, msg, val, val2)
#endif

#define FGLCD_REP_2(code) do { code; code; } while(0)
#define FGLCD_REP_3(code) do { code; code; code; } while(0)
#define FGLCD_REP_4(code) do { code; code; code; code; } while(0)
#define FGLCD_REP_5(code) do { code; code; code; code; code; } while(0)
#define FGLCD_REP_6(code) do { code; code; code; code; code; code; } while(0)
#define FGLCD_REP_8(code) do {  FGLCD_REP_4(code); FGLCD_REP_4(code); } while(0)
#define FGLCD_REP_16(code) do {  FGLCD_REP_8(code); FGLCD_REP_8(code); } while(0)
#define FGLCD_REP_32(code) do {  FGLCD_REP_16(code); FGLCD_REP_16(code); } while(0)

#if 0

// old version -- generates jump table and usually larger code and somtimes spurious/unnecessary movs

#define FGLCD_DUFF4(T, N, code)      \
  do {                       \
    T n = (T)(((N) + 3u) / 4u); \
    switch ((N) % 4u) {        \
      case 0: do { code;   \
      case 3:      code;   \
      case 2:      code;   \
      case 1:      code;   \
      } while (--n);       \
    }} while(0)
    
#define FGLCD_DUFF8(T, N, code)      \
  do {                       \
    T n = (T)(((N) + 7u) / 8u); \
    switch ((N) % 8u) {        \
      case 0: do { code;   \
      case 7:      code;   \
      case 6:      code;   \
      case 5:      code;   \
      case 4:      code;   \
      case 3:      code;   \
      case 2:      code;   \
      case 1:      code;   \
      } while (--n);       \
    }} while(0)
    
#define FGLCD_DUFF16(T, N, code)      \
  do {                       \
    T n = (T)(((N) + 15u) / 16u); \
    switch ((N) % 16u) {        \
      case 0: do { code;   \
      case 15:      code;  \
      case 14:      code;  \
      case 13:      code;  \
      case 12:      code;  \
      case 11:      code;  \
      case 10:      code;  \
      case 9:      code;   \
      case 8:      code;   \
      case 7:      code;   \
      case 6:      code;   \
      case 5:      code;   \
      case 4:      code;   \
      case 3:      code;   \
      case 2:      code;   \
      case 1:      code;   \
      } while (--n);       \
    }} while(0)

#define FGLCD_DUFF32(T, N, code)      \
  do {                       \
    T n = (T)(((N) + 31u) / 32u); \
    switch ((N) % 32u) {        \
      case 0: do { code;   \
      case 31:      code;  \
      case 30:      code;  \
      case 29:      code;  \
      case 28:      code;  \
      case 27:      code;  \
      case 26:      code;  \
      case 25:      code;  \
      case 24:      code;   \
      case 23:      code;   \
      case 22:      code;   \
      case 21:      code;   \
      case 20:      code;   \
      case 19:      code;   \
      case 18:      code;   \
      case 17:      code;   \
      case 16:      code;   \
      case 15:      code;  \
      case 14:      code;  \
      case 13:      code;  \
      case 12:      code;  \
      case 11:      code;  \
      case 10:      code;  \
      case 9:      code;   \
      case 8:      code;   \
      case 7:      code;   \
      case 6:      code;   \
      case 5:      code;   \
      case 4:      code;   \
      case 3:      code;   \
      case 2:      code;   \
      case 1:      code;   \
      } while (--n);       \
    }} while(0)

#else

// new version -- generates much better code
// also renamed internal vars to make sure there are no name conflicts;
// as otherwise the gcc optimizer acts weird and doesn't actually unroll...?

#define FGLCD_DUFF4(T, N, code)      \
  do {                       \
    uint8_t _DUFF_r = uint8_t((N) & 3u); \
    while(_DUFF_r--) { code; } \
    T _DUFF_n = (T)((N) & ~3u); \
    while(_DUFF_n) { FGLCD_REP_4(code); _DUFF_n -= 4; } \
 } while(0)

 #define FGLCD_DUFF8(T, N, code)      \
  do {                       \
    uint8_t _DUFF_r = uint8_t((N) & 7u); \
    while(_DUFF_r--) { code; } \
    T _DUFF_n = (T)((N) & ~7u); \
    while(_DUFF_n) { FGLCD_REP_8(code); _DUFF_n -= 8; } \
 } while(0)

  #define FGLCD_DUFF16(T, N, code)      \
  do {                       \
    uint8_t _DUFF_r = uint8_t((N) & 15u); \
    while(_DUFF_r--) { code; } \
    T _DUFF_n = (T)((N) & ~15u); \
    while(_DUFF_n) { FGLCD_REP_16(code); _DUFF_n -= 16; } \
 } while(0)

  #define FGLCD_DUFF32(T, N, code)      \
  do {                       \
    uint8_t _DUFF_r = uint8_t(uint8_t(N) & 3u); \
    while(_DUFF_r--) { code; } \
    _DUFF_r = uint8_t(uint8_t(N) & (31u & ~3u)); \
    while(_DUFF_r) { FGLCD_REP_4(code); _DUFF_r -= 4; } \
    T _DUFF_n = (T)((N) & ~31u); \
    while(_DUFF_n) { FGLCD_REP_32(code); _DUFF_n -= 32; } \
 } while(0)


  #define FGLCD_DUFF32_DIV(T, N, code)      \
  do {                       \
    uint8_t _DUFF_r = uint8_t(uint8_t(N) % 32u); \
    while(_DUFF_r--) { code; } \
    _DUFF_r = uint8_t(uint8_t(N) & (31u & ~3u)); \
    while(_DUFF_r) { FGLCD_REP_4(code); _DUFF_r -= 4; } \
    T _DUFF_n = (T)((N) / 32u); \
    while(_DUFF_n--) { FGLCD_REP_32(code); } \
 } while(0)

#endif
