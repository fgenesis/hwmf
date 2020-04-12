#pragma once

#include "mcu.h"

namespace fglcd {
namespace seg7 {


enum NBit
{
  U = 1,
  UR = 2,
  BR = 4,
  B = 8,
  BL = 16,
  UL = 32,
  M = 64,
  DOT = 128,
  ALLN = 0xff - DOT
};

static const u8 D_num[] PROGMEM =
{
  /* 0 */ ALLN - M,
  /* 1 */ UR|BR,
  /* 2 */ U|UR|M|BL|B,
  /* 3 */ U|UR|M|BR|B,
  /* 4 */ UL|M|UR|BR,
  /* 5 */ U|UL|M|BR|B,
  /* 6 */ ALLN - UR,
  /* 7 */ U|UR|BR,
  /* 8 */ ALLN,
  /* 9 */ ALLN - BL
};

static const u8 D_al[] PROGMEM =
{
  /* a */ ALLN - B,
  /* b */ ALLN - U - UR,
  /* c */ ALLN - UR-M-BR,
  /* d */ ALLN - U-UL,
  /* e */ ALLN - UR-BR,
  /* f */ U|UL|M|BL,
  /* g */ ALLN - UR-M,
  /* h */ ALLN - U-B,
  /* i */ BL | U,
  /* j */ B|BR|UR|BL,
  /* k */ UL|BL|M|UR|B, // sucks
  /* l */ UL|BL|B,
  /* m */ ALLN - B-M, // sucks
  /* n */ BL|M|BR,
  /* o */ M|B|BL|BR,
  /* p */ ALLN - B-BR,
  /* q */ ALLN - BL-B,
  /* r */ BL|M,
  /* s */ ALLN-UR-BL,
  /* t */ UL|M|BL|B,
  /* u */ ALLN - U-M,
  /* v */ BL|B|BR,
  /* w */ ALLN - U, // sucks
  /* x */ UR|M|BL, // sucks
  /* y */ ALLN - U-BL,
  /* z */ ALLN - UL-BR + DOT,
};

static const u8 D_specialLUT[] PROGMEM = {   ' ', '_',  '=',  '|',   '^',     '-', ',',   '/',       '\\',    '\'', '.',   ']',      '[',          ')',       '('      };
static const u8 D_special[]    PROGMEM = {    0,   B,   B|M, BR|BL, UL|U|UR,   M,  BR|B,  UR|M|BL,  UL|M|BR,   UR,  DOT,  U|UR|BR|B, U|UL|BL|B,  U|UR|BR|B, U|UL|BL|B  };

static u8 transform(u8 c)
{
  if(c >= '0' && c <= '9')
    return memtype::Progmem::read<u8>(&D_num[c - '0']);
  if(c <= 9)
    return memtype::Progmem::read<u8>(&D_num[c]);
  if(c <= 0xf)
    return memtype::Progmem::read<u8>(&D_al[c]);
  if(c >= 'a' && c <= 'z')
    return memtype::Progmem::read<u8>(&D_al[c - 'a']);
  if(c >= 'A' && c <= 'Z')
    return memtype::Progmem::read<u8>(&D_al[c - 'A']);

  for(u8 i = 0; i < sizeof(D_specialLUT); ++i)
    if(c == memtype::Progmem::read<u8>(&D_specialLUT[i]))
      return memtype::Progmem::read<u8>(&D_special[i]);

  return U|M|B;
}


}
}
