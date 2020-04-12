#pragma once

enum PackType
{
    PK_FAIL = -1,
    // lowlevel algos
    PK_TSCHUNK = 0,
    PK_SRLE,
    PK_UNCOMP,
    PK_RLE,
    PK_EXO,
    // higher level composite algos come afterwards
    PK_BLOCK_TSCHUNK,
    PK_BLOCK_MIX,
    PK_BLOCK_EXO,
    PK_BLOCK_RLE,
};

#define PK_NUM_BASIC_ALGOS 5

static const char * const PackTypeName[] =
{
    "PK_TSCHUNK",
    "PK_SRLE",
    "PK_UNCOMP",
    "PK_RLE",
    "PK_EXO",
    "PK_BLOCK_TSCHUNK",
    "PK_BLOCK_MIX",
    "PK_BLOCK_EXO",
    "PK_BLOCK_RLE"
};

static const char * const PackTypeNameShort[] = { "tsc", "srl", "raw", "rle", "exo", "Bt", "Bm", "Be", "Br" };

enum PackTypeProps_ // bitmask
{
    PP_HAS_SIZE = 1, // needs size externally supplied, aka has no end marker
    PP_COMPOUND = 2, // method combines other method
    PP_HAS_WINDOW = 4, // depacker needs memory window and that window size is relevant for compression
    PP_NON_RELOCATABLE = 8, // depacker depends on previously packed content. compressed data may not be relocatable.
};

static constexpr unsigned PackTypeProps[] =
{
    PP_HAS_SIZE | PP_NON_RELOCATABLE,
    PP_HAS_SIZE | PP_NON_RELOCATABLE,
    PP_HAS_SIZE,
    0,
    PP_HAS_WINDOW,

    PP_COMPOUND,
    PP_COMPOUND,
    PP_COMPOUND,
    PP_COMPOUND
};
