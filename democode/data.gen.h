#pragma once
#include "data.def.h"

extern const uint8_t data_exo_a2560_path[120] PROGMEM_FAR;
extern const uint8_t data_exo_a2560_title[131] PROGMEM_FAR;
extern const uint8_t data_exo_cbm1[961] PROGMEM_FAR;
extern const uint8_t data_exo_cbm2[942] PROGMEM_FAR;
extern const uint8_t data_exo_credits[120] PROGMEM_FAR;
extern const uint8_t data_exo_greets[437] PROGMEM_FAR;
extern const uint8_t data_exo_hwmf[581] PROGMEM_FAR;
extern const uint8_t data_exo_hwmf_cross[49] PROGMEM_FAR;
extern const uint8_t data_exo_hwmf_cube[61] PROGMEM_FAR;
extern const uint8_t data_exo_hwmf_nameoverlay[122] PROGMEM_FAR;
extern const uint8_t data_exo_hwmf_path_cross[45] PROGMEM_FAR;
extern const uint8_t data_exo_hwmf_path_cube[44] PROGMEM_FAR;
extern const uint8_t data_exo_hwmf_path_rmcc[61] PROGMEM_FAR;
extern const uint8_t data_exo_intro1[56] PROGMEM_FAR;
extern const uint8_t data_exo_intro2[72] PROGMEM_FAR;
extern const uint8_t data_exo_paths[422] PROGMEM_FAR;
extern const uint8_t data_exo_specs[317] PROGMEM_FAR;
extern const uint8_t data_exo_theend[138] PROGMEM_FAR;
struct data_3x5_glf
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\glfconvert.exe 3x5.glf greets.txt

static const unsigned paloffs = 0;
static const bool blocked = false;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 291;
static const unsigned ndata = 213;
static const unsigned char data[213] PROGMEM_FAR;
static const unsigned packedsize = 213;
static const unsigned fullsize = 482;
static const unsigned nusedch = 58;
static const uint8_t usedch[58] PROGMEM_FAR;
static const unsigned noffsets = 58;
static const uint16_t offsets[58] PROGMEM_FAR;
static const uint8_t fontheight = 7;
};
struct data_topaz_glf
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\glfconvert.exe topaz.glf guru.txt

static const unsigned paloffs = 0;
static const bool blocked = false;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 551;
static const unsigned ndata = 268;
static const unsigned char data[268] PROGMEM_FAR;
static const unsigned packedsize = 268;
static const unsigned fullsize = 584;
static const unsigned nusedch = 29;
static const uint8_t usedch[29] PROGMEM_FAR;
static const unsigned noffsets = 29;
static const uint16_t offsets[29] PROGMEM_FAR;
static const uint8_t fontheight = 18;
};
struct data_vga_glf
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\glfconvert.exe vga.glf intro1.txt intro2.txt credits.txt theend.txt

static const unsigned paloffs = 0;
static const bool blocked = false;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 636;
static const unsigned ndata = 368;
static const unsigned char data[368] PROGMEM_FAR;
static const unsigned packedsize = 368;
static const unsigned fullsize = 769;
static const unsigned nusedch = 46;
static const uint8_t usedch[46] PROGMEM_FAR;
static const unsigned noffsets = 46;
static const uint16_t offsets[46] PROGMEM_FAR;
static const uint8_t fontheight = 18;
};
struct data_wire_wolf_gif
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe -p exo wire_wolf.gif

static const unsigned paloffs = 0;
static const bool blocked = false;
static const uint32_t fullsize = 100160;
static const unsigned w = 313;
static const unsigned h = 320;
static const unsigned npal = 115;
static const unsigned short pal[115] PROGMEM_FAR;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 255;
static const unsigned ndata = 27211;
static const unsigned char data[27211] PROGMEM_FAR;
static const unsigned packedsize = 27211;
static const bool useRLERecomp = false;
};
struct data_cityscroll1_gif
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe -b 1 0 -s 5 cityscroll1.gif

static const unsigned paloffs = 0;
static const bool blocked = true;
static const uint32_t fullsize = 253760;
static const unsigned w = 793;
static const unsigned h = 320;
static const unsigned npal = 16;
static const unsigned short pal[16] PROGMEM_FAR;
static const unsigned blockw = 1;
static const unsigned blockh = 320;
static const PackType packtype = PK_BLOCK_TSCHUNK;
static const unsigned windowsize = 0;
static const unsigned ndata = 25359;
static const unsigned char data[25359] PROGMEM_FAR;
static const unsigned packedsize = 25359;
static const bool useRLERecomp = false;
};
struct data_cityscroll2_gif
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe -b 1 0 -s 5 cityscroll2.gif

static const unsigned paloffs = 0;
static const bool blocked = true;
static const uint32_t fullsize = 258240;
static const unsigned w = 807;
static const unsigned h = 320;
static const unsigned npal = 19;
static const unsigned short pal[19] PROGMEM_FAR;
static const unsigned blockw = 1;
static const unsigned blockh = 320;
static const PackType packtype = PK_BLOCK_TSCHUNK;
static const unsigned windowsize = 0;
static const unsigned ndata = 19853;
static const unsigned char data[19853] PROGMEM_FAR;
static const unsigned packedsize = 19853;
static const bool useRLERecomp = false;
};
struct data_github_png
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe github.png

static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned fullsize = 1024;
static const unsigned w = 32;
static const unsigned h = 32;
static const unsigned npal = 2;
static const unsigned short pal[2] PROGMEM_FAR;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 241;
static const unsigned ndata = 109;
static const unsigned char data[109] PROGMEM_FAR;
static const unsigned packedsize = 109;
static const bool useRLERecomp = false;
};
struct data_mdscroll_png
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe -b 0 1 -s 5 -p exo mdscroll.png

static const unsigned paloffs = 0;
static const bool blocked = true;
static const unsigned fullsize = 53120;
static const unsigned w = 166;
static const unsigned h = 320;
static const unsigned npal = 10;
static const unsigned short pal[10] PROGMEM_FAR;
static const unsigned blockw = 166;
static const unsigned blockh = 1;
static const PackType packtype = PK_BLOCK_EXO;
static const unsigned windowsize = 129;
static const unsigned ndata = 16520;
static const unsigned char data[16520] PROGMEM_FAR;
static const unsigned packedsize = 16520;
static const bool useRLERecomp = false;
};
struct data_pcb64_png
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe pcb64.png

static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned fullsize = 4096;
static const unsigned w = 64;
static const unsigned h = 64;
static const unsigned npal = 14;
static const unsigned short pal[14] PROGMEM_FAR;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 255;
static const unsigned ndata = 665;
static const unsigned char data[665] PROGMEM_FAR;
static const unsigned packedsize = 665;
static const bool useRLERecomp = false;
};
struct data_screenborder_png
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe screenborder.png

static const unsigned paloffs = 0;
static const bool blocked = false;
static const uint32_t fullsize = 153600;
static const unsigned w = 480;
static const unsigned h = 320;
static const unsigned npal = 14;
static const unsigned short pal[14] PROGMEM_FAR;
static const PackType packtype = PK_TSCHUNK;
static const unsigned windowsize = 0;
static const unsigned ndata = 1790;
static const unsigned char data[1790] PROGMEM_FAR;
static const unsigned packedsize = 1790;
static const bool useRLERecomp = false;
};
struct data_sideup_png
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe -p exo sideup.png

static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned fullsize = 8192;
static const unsigned w = 128;
static const unsigned h = 64;
static const unsigned npal = 2;
static const unsigned short pal[2] PROGMEM_FAR;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 253;
static const unsigned ndata = 280;
static const unsigned char data[280] PROGMEM_FAR;
static const unsigned packedsize = 280;
static const bool useRLERecomp = false;
};
struct data_tiles_gif
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\imgpack.exe -b 32 1 -p tsc tiles.gif

static const unsigned paloffs = 0;
static const bool blocked = true;
static const unsigned fullsize = 11264;
static const unsigned w = 32;
static const unsigned h = 352;
static const unsigned npal = 49;
static const unsigned short pal[49] PROGMEM_FAR;
static const unsigned blockw = 32;
static const unsigned blockh = 1;
static const PackType packtype = PK_BLOCK_TSCHUNK;
static const unsigned windowsize = 0;
static const unsigned ndata = 4535;
static const unsigned char data[4535] PROGMEM_FAR;
static const unsigned packedsize = 4535;
static const bool useRLERecomp = false;
};
struct data_cockpit_obj
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\meshpack.exe -m 4095 cockpit.obj

static constexpr inline fp1616 scale() { return fp1616::raw(1996809); }
static const unsigned Ntris = 78;
static const unsigned Nverts = 41;
static const bool Indexed = true;
static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned npal = 5;
static const unsigned short pal[5] PROGMEM_FAR;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 124;
static const unsigned ndata = 416;
static const unsigned char data[416] PROGMEM_FAR;
static const unsigned packedsize = 416;
static const unsigned fullsize = 558;
};
struct data_cube_obj
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\meshpack.exe -m 4095 cube.obj

static constexpr inline fp1616 scale() { return fp1616::raw(65536); }
static const unsigned Ntris = 12;
static const unsigned Nverts = 8;
static const bool Indexed = true;
static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned npal = 3;
static const unsigned short pal[3] PROGMEM_FAR;
static const PackType packtype = PK_TSCHUNK;
static const unsigned windowsize = 0;
static const unsigned ndata = 70;
static const unsigned char data[70] PROGMEM_FAR;
static const unsigned packedsize = 70;
static const unsigned fullsize = 96;
};
struct data_icosahedron_obj
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\meshpack.exe -m 4095 icosahedron.obj

static constexpr inline fp1616 scale() { return fp1616::raw(5062144); }
static const unsigned Ntris = 20;
static const unsigned Nverts = 12;
static const bool Indexed = true;
static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned npal = 0;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 64;
static const unsigned ndata = 109;
static const unsigned char data[109] PROGMEM_FAR;
static const unsigned packedsize = 109;
static const unsigned fullsize = 152;
};
struct data_revisionflat_obj
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\meshpack.exe -m 4095 -q 0.75 revisionflat.obj

static constexpr inline fp1616 scale() { return fp1616::raw(1458435); }
static const unsigned Ntris = 132;
static const unsigned Nverts = 132;
static const bool Indexed = true;
static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned npal = 1;
static const unsigned short pal[1] PROGMEM_FAR;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 630;
static const unsigned ndata = 1024;
static const unsigned char data[1024] PROGMEM_FAR;
static const unsigned packedsize = 1024;
static const unsigned fullsize = 1320;
};
struct data_sofa_obj
{
// Generated via:  E:\code\demo\avrdemo\demo1\data\meshpack.exe -m 4095 -q 3 sofa.obj

static constexpr inline fp1616 scale() { return fp1616::raw(170669); }
static const unsigned Ntris = 102;
static const unsigned Nverts = 78;
static const bool Indexed = true;
static const unsigned paloffs = 0;
static const bool blocked = false;
static const unsigned npal = 6;
static const unsigned short pal[6] PROGMEM_FAR;
static const PackType packtype = PK_EXO;
static const unsigned windowsize = 276;
static const unsigned ndata = 583;
static const unsigned char data[583] PROGMEM_FAR;
static const unsigned packedsize = 583;
static const unsigned fullsize = 876;
};
extern const uint8_t data_raw_guru[98+1] PROGMEM_FAR;
extern const uint8_t data_raw_intro1[29+1] PROGMEM_FAR;
