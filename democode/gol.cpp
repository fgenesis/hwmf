#include "demoshared.h"

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif

#define GREETINGS_SIZE 600 // gotta be enough

#define DELAY_MS(x) fglcd::delay_ms(x)

static const u8 BORDER = 1;
static const u8 YSTART = 1;
static const u8 XSTART = 1;

static const u8 BLOCKSIZE = 4;
static const u8 NPIX = BLOCKSIZE * BLOCKSIZE;

const u16 WR = u16(LCD::WIDTH - XSTART - BORDER) / u16(BLOCKSIZE+BORDER);
const u16 W = WR / 4;
const u16 H = u16(LCD::HEIGHT - YSTART - BORDER) / u16(BLOCKSIZE+BORDER);

const u16 NX = 2 + W;
const u16 NY = 2 + H;
const u16 NBUF = NX * H;


// compresses bits [aabbccdd] into [0000abcd] (00 -> 0, otherwise 1)
static const u8 compressbits[] PROGMEM_FAR = { // copied to scratch2
0, 1, 1, 1, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 4, 5, 5, 5, 6, 7, 7, 7, 6, 7, 7, 7, 6, 7, 7, 7, 4, 5, 5, 5, 6, 7, 7, 7, 6, 7, 7, 7, 6, 7, 7, 7, 4, 5, 5, 5, 6, 7, 7, 7, 6, 7, 7, 7, 6, 7, 7, 7, 8, 9, 9, 9, 10, 11, 11, 11, 10, 11, 11, 11, 10, 11, 11, 11, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15, 8, 9, 9, 9, 10, 11, 11, 11, 10, 11, 11, 11, 10, 11, 11, 11, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15, 8, 9, 9, 9, 10, 11, 11, 11, 10, 11, 11, 11, 10, 11, 11, 11, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15, 12, 13, 13, 13, 14, 15, 15, 15, 14, 15, 15, 15, 14, 15, 15, 15
};
// for each aa,bb,cc,dd compute (00 -> 01, 01 -> 10, 10 -> 11, 11 -> 11)
static const u8 advancebits[] PROGMEM_FAR = { // copied to scratch3
85, 86, 87, 87, 89, 90, 91, 91, 93, 94, 95, 95, 93, 94, 95, 95, 101, 102, 103, 103, 105, 106, 107, 107, 109, 110, 111, 111, 109, 110, 111, 111, 117, 118, 119, 119, 121, 122, 123, 123, 125, 126, 127, 127, 125, 126, 127, 127, 117, 118, 119, 119, 121, 122, 123, 123, 125, 126, 127, 127, 125, 126, 127, 127, 149, 150, 151, 151, 153, 154, 155, 155, 157, 158, 159, 159, 157, 158, 159, 159, 165, 166, 167, 167, 169, 170, 171, 171, 173, 174, 175, 175, 173, 174, 175, 175, 181, 182, 183, 183, 185, 186, 187, 187, 189, 190, 191, 191, 189, 190, 191, 191, 181, 182, 183, 183, 185, 186, 187, 187, 189, 190, 191, 191, 189, 190, 191, 191, 213, 214, 215, 215, 217, 218, 219, 219, 221, 222, 223, 223, 221, 222, 223, 223, 229, 230, 231, 231, 233, 234, 235, 235, 237, 238, 239, 239, 237, 238, 239, 239, 245, 246, 247, 247, 249, 250, 251, 251, 253, 254, 255, 255, 253, 254, 255, 255, 245, 246, 247, 247, 249, 250, 251, 251, 253, 254, 255, 255, 253, 254, 255, 255, 213, 214, 215, 215, 217, 218, 219, 219, 221, 222, 223, 223, 221, 222, 223, 223, 229, 230, 231, 231, 233, 234, 235, 235, 237, 238, 239, 239, 237, 238, 239, 239, 245, 246, 247, 247, 249, 250, 251, 251, 253, 254, 255, 255, 253, 254, 255, 255, 245, 246, 247, 247, 249, 250, 251, 251, 253, 254, 255, 255, 253, 254, 255, 255
};
// how many bits set in compressed [0000abcd] as [0yyy0xxx] where x,y are counts
// (x looks at bits abc, y looks at bits bcd)
static const u8 compressedcounts_all_2x[] PROGMEM_FAR = {
0, 1, 17, 18, 17, 18, 34, 35, 16, 17, 33, 34, 33, 34, 50, 51
};
// own cell isn't included in the count (x looks at bits ac, y looks at bits bd)
static const u8 compressedcounts_outer_2x[] PROGMEM_FAR = {
0, 1, 16, 17, 1, 2, 17, 18, 16, 17, 32, 33, 17, 18, 33, 34
};
static const u8 compressedcounts_single[] PROGMEM_FAR = {
0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};
// given [0000abcd], return mask to AND with, so that if bit a..d is set, aa..dd will be set, otherwise 00 each
static const u8 uncompressmask[] PROGMEM_FAR = {
0, 3, 12, 15, 48, 51, 60, 63, 192, 195, 204, 207, 240, 243, 252, 255
};

static const u8 alive[] PROGMEM_FAR = {
    0, 0, // Any live cell with fewer than two live neighbours dies, as if caused by underpopulation.
    1, // Any live cell with two or three live neighbours lives on to the next generation.
    1|(1<<4), // Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
    0, 0, 0, 0, 0 // Any live cell with more than three live neighbours dies, as if by overpopulation.
};

static FORCEINLINE u8 sumlo(u8 a, u8 b, u8 c)
{
    u8 r = a & 1;
    r += b & 1;
    r += c & 1;
    return r;
}

static FORCEINLINE u8 sumhi4(u8 a, u8 b, u8 c)
{
    u8 r = a & 0x8;
    r += b & 0x8;
    r += c & 0x8;
    return r >> 3;
}

// scratch0/1 [0..3] hold the palette data, so anything after that is free
#ifdef _MSC_VER // fuckings to visual studio
#define ptr_compressedcounts_all_2x  (scratch0 + 4)
#define ptr_compressedcounts_outer_2x (ptr_compressedcounts_all_2x + sizeof(compressedcounts_all_2x))
#define ptr_compressedcounts_single (ptr_compressedcounts_outer_2x + sizeof(compressedcounts_outer_2x))
#define ptr_uncompressmask (ptr_compressedcounts_single + sizeof(compressedcounts_single))
#define ptr_alive (ptr_uncompressmask + sizeof(uncompressmask))
#else
static constexpr u8 * ptr_compressedcounts_all_2x = scratch0 + 4;
static constexpr u8 * ptr_compressedcounts_outer_2x = ptr_compressedcounts_all_2x + sizeof(compressedcounts_all_2x);
static constexpr u8 * ptr_compressedcounts_single = ptr_compressedcounts_outer_2x + sizeof(compressedcounts_outer_2x);
static constexpr u8 * ptr_uncompressmask = ptr_compressedcounts_single + sizeof(compressedcounts_single);
static constexpr u8 * ptr_alive = ptr_uncompressmask + sizeof(uncompressmask);
static_assert(ptr_alive + sizeof(alive) < scratch1, "oops");
#endif


/* // old progmem based lookup
#define RP(x, i) (pgm_read_byte(&x[i]))
#define compr(k) RP(compressbits, k)
#define advance(k) RP(advancebits, k)
#define bcount(k) RP(compressedcounts_single, k)
#define cc2ax(k) RP(compressedcounts_all_2x, k)
#define cc2ox(k) RP(compressedcounts_outer_2x, k)
#define isalive(k) RP(alive, k);
#define uncompr(k) RP(uncompressmask, k);
*/

// now with scratchpad memory
#define compr(k) pagelookup<u8>(scratch2, k)
#define advance(k) pagelookup<u8>(scratch3, k)
#define bcount(k) pagelookup<u8>(ptr_compressedcounts_single, k)
#define cc2ax(k) pagelookup<u8>(ptr_compressedcounts_all_2x, k)
#define cc2ox(k) pagelookup<u8>(ptr_compressedcounts_outer_2x, k)
#define isalive(k) pagelookup<u8>(ptr_alive, k)
#define uncompr(k) pagelookup<u8>(ptr_uncompressmask, k)


struct GolState
{
    GolState();

    u8 **rows;
    u8 w, h;
    TinyRng16 rng;
    u8 *wrk[2];
    bool randomize;
    bool musicvis;
    bool done;
    bool fulldraw;

    volatile u8 paused;
    u8 fontlines;

    u8 fontidx[256]; // scratch pages are completely occupied, so use an extra buf
    char greets[GREETINGS_SIZE];

    // for the draw-to-state-thing

    typedef bool ColorType;
    typedef u8 DimType;

    static const DimType WIDTH = NX;

    typedef DrawFont<data_3x5_glf, GolState> TheFont;
    TheFont font;
    FontTyper<TheFont> typer;


    bool _drawon;
    u8 _drawx, _drawy, _drawxmin, _drawymin, _drawxmax, _drawymax; // in sub-coords

    FORCEINLINE void setColor(bool col)
    {
        _drawon = col;
    }

    FORCEINLINE void setpoint(u8 x, u8 y)
    {
        u8 mask = 3 << (x & 3);
        rows[y][x >> 2] |= mask;

        constexpr u16 inc = BLOCKSIZE + BORDER;
        constexpr Color c = LCD::gencolor_inl(0xff,0x22,0);
        LCD::fillrect(BORDER + x * inc, BORDER + y * inc, BLOCKSIZE, BLOCKSIZE, c);
    }

    FORCEINLINE void fastfill_u8(u8 n)
    {
        const u8 xmin = _drawxmin;
        const u8 ymin = _drawymin;
        const u8 xmax = _drawxmax;
        const u8 ymax = _drawymax;
        u8 x = _drawx;
        u8 y = _drawy;
        bool on = _drawon;
        while(n--)
        {
            if(on)
                setpoint(x,y);
            ++x;
            if(x > xmax)
            {
                x = xmin;
                ++y;
                FGLCD_ASSERT(y <= ymax, "golpffy");
            }
        }
        _drawx = x;
        _drawy = y;
    }

    FORCEINLINE void setxywh(DimType x, DimType y, DimType w, DimType h)
    {
        _drawx = x;
        _drawy = y;
        _drawxmin = x;
        _drawymin = y;
        _drawxmax = x + w - 1;
        _drawymax = y + h - 1;
    }
};

static void ev_unpause(void *ud)
{
    GolState& gol = *(GolState*)ud;
    gol.typer.setPos(4,4);
    gol.paused = 0;
    gol.fulldraw = true;
}


static uint16_t fontcb(char& c, LCD::PixelPos pp, void *ud)
{
    GolState& gol = *(GolState*)ud;
    gol.paused = true;
    u16 d = 20;
    if(c == '\n' || c == 0)
    {
        d = 200;
        u8 ln = gol.fontlines + 1;
        gol.fontlines = ln;
        if(ln >= 7 || c == 0)
        {
            ln = 0;
            evs::schedule(1222, ev_unpause, ud);
            d = 2666;
        }
        gol.fontlines = ln;
    }
    return c ? d : 0;
}

GolState::GolState()
    : font(fontidx, this)
    , typer(font, greets, fontcb, this)
{
    fontlines = 0;
    fulldraw = false;

    decompressRAM<PK_EXO, sizeof(greets), fglcd::ProgmemFar>(&greets[0], fglcd_get_farptr(data_exo_greets), sizeof(data_exo_greets));

    font.rebuildIndex(); // clobbered by decomp

    ev_unpause(this); // clean reset
    paused = true;
    typer.start(10);
}


static FORCEINLINE void drawcell(const u8 bits, u16 x, u16 y)
{
    LCD::setxywh_inl(x, y, BLOCKSIZE, BLOCKSIZE);
    //LCD::setColor(bits ? 0xffff : 0);
    LCD::setColor(palgetcolor_inl(bits));
    //LCD::setColor(LCD::gencolor(x, y, bits ? 0xff : 0));
    LCD::fastfill_inl(NPIX);
}

static FORCEINLINE void updaterow(u8 * const next, const u8 * const row, const u8 * const above, const u8 *const below, GolState& gol, const u16 drawy)
{
    const u8 W = gol.w;
    u16 drawx = XSTART;
    const u16 xdrawfull = (BLOCKSIZE + BORDER) * 4u;
    for(u8 x = 0; x < W; ++x)
    {
        // layout:
        // lo                 hi
        // [L0] [   U    ]  [R0]
        // [L]  [aabbccdd]  [R]
        // [L1] [   D    ]  [R1]
        // ^------------------- need 2 highest bits of L
        //                  ^-- need 2 lowest bits of R
        const u8 C = row[x];     // [aabbccdd]
        const u8 cC = compr(C);  // [0000abcd]
        // update center 2 cells (B and C)
        const u8 U = above[x];
        const u8 D = below[x];
        const u8 cU = compr(U);
        const u8 cD = compr(D);
        const u8 nC = cc2ox(cC);
        const u8 nU = cc2ax(cU);
        const u8 nD = cc2ax(cD);
        const u8 n12 = nC + nU + nD; // parallel 2x 4 bit add
        const u8 nb = n12 & 0xf;
        const u8 nc = n12 >> 4u;

        // count outer cells (for a and d)
        const u8 L0 = above[x-1];
        const u8 L = row[x-1];
        const u8 L1 = below[x-1];
        const u8 R0 = above[x+1];
        const u8 R = row[x+1];
        const u8 R1 = below[x+1];
#ifdef TESTMODE
        if(!x)
            ASSERT(!L && !L0 && !L1);
        if(x+1 == W)
            ASSERT(!R && !R0 && !R1);
#endif
        const u8 cL0 = compr(L0);
        const u8 cL = compr(L);
        const u8 cL1 = compr(L1);
        const u8 cR0 = compr(R0);
        const u8 cR = compr(R);
        const u8 cR1 = compr(R1);

        // counts for low bits of R and L's low nybble's hi bit
        const u8 nL = sumhi4(cL0, cL, cL1);
        const u8 nR = sumlo(cR0, cR, cR1);
        const u8 bits_bcd = cC >> 1u; // drop "a" bit
        const u8 nxCa = bits_bcd & 1; // isolate b bit
        const u8 nxCd = (bits_bcd >> 1u) & 1; // isolate c bit
        const u8 nxUa = bcount(cU & 3);
        const u8 nxDa = bcount(cD & 3);
        const u8 na = nL + nxCa + nxUa + nxDa;
        const u8 nxUd = bcount(cU & 12);
        const u8 nxDd = bcount(cD & 12);
        const u8 nd = nR + nxCd + nxUd + nxDd;

        // upper 4 bits are unconditionally applied (always alive), lower 4 bits depend on current state (stay alive)
        u8 cx = isalive(nd);
        cx <<= 1u;
        cx |= isalive(nc);
        cx <<= 1u;
        cx |= isalive(nb);
        cx <<= 1u;
        cx |= isalive(na);
        //               become alive  | stay alive
        const u8 lifebits = (cx >> 4u) | (cx & cC);

        const u8 lifemask = uncompr(lifebits);
        const u8 cadv = advance(C);
        const u8 cnext = cadv & lifemask;
        next[x] = cnext;

        // -------- draw -------------
        u8 cell = cnext;
        const u8 xinc = BLOCKSIZE + BORDER;
        u16 dx = drawx;
        if(UNLIKELY(gol.fulldraw))
        {
            u8 k = 0;
            for( ; k < 4; cell >>= 2u, dx += xinc, ++k)
                drawcell(cell & 3, dx, drawy);
        }
        else if(u8 changebits = compr(cnext ^ C))
        {
            for( ; changebits; cell >>= 2u, dx += xinc, changebits >>= 1u)
                if(changebits & 1)
                    drawcell(cell & 3, dx, drawy);
        }

        drawx += xdrawfull;
    }
}

static FORCEINLINE void updatefield(GolState& gol)
{
    u8 **rows = &gol.rows[0];
    u8 **wrk = &gol.wrk[0];
    const u8 W = gol.w;
    const u8 H = gol.h;
    u16 drawy = YSTART;
    // invariants:
    // wrk[0] is written to
    // wrk[1] contains above row as it was before it was updated 
    // zero rows at rows[-1] and rows[W-1] are never touched or exchanged

    {
        const u8 * const zero = rows[-1];
        u8 * const newrow = wrk[0];
        u8 * const tmp = wrk[1]; // unused
        u8 * const oldrow = rows[0];
        const u8 * const below = rows[1]; // yet untouched
        updaterow(newrow, oldrow, zero, below, gol, drawy);
        wrk[0] = tmp;
        wrk[1] = oldrow;
        rows[0] = newrow;
    }

    for(u8 y = 1; y < H; ++y)
    {
        drawy += BLOCKSIZE + BORDER;
        u8 * const newrow = wrk[0];
        u8 * const oldrow = rows[y];
        u8 * const oldabove = wrk[1];
        const u8 * const below = rows[y+1]; // yet untouched
        updaterow(newrow, oldrow, oldabove, below, gol, drawy);
        wrk[0] = oldabove; // don't need this anymore
        wrk[1] = oldrow; // will be next row's oldabove
        rows[y] = newrow; // just written data
    }
}

/*
static FORCEINLINE void drawline(const Color *pal, const u8 * const row, const u8 W, const u8 sz, u16 x, const u16 y)
{
    const u16 N = u16(sz)*sz;
    const u8 xinc = sz+BORDER;
    for(u8 i = 0; i < W; ++i)
    {
        u8 cell = row[i];
        for(u8 j = 0; j < 4; ++j, cell >>= 2u, x += xinc)
            drawcell(pal, cell & 3, x, y, sz, N);
    }
#ifdef TESTMODE
    putchar('\n');
#endif
}

static NOINLINE void drawfield(const Color *pal, const u8 * const * const rows, const u8 W, const u8 H, const u8 sz, const u16 x, u16 y)
{
#ifdef TESTMODE
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD co { 0, 0 };
    SetConsoleCursorPosition(h, co);
#endif

    const u8 yinc = sz+BORDER;
    for(u8 i = 0; i < H; ++i, y += yinc)
        drawline(pal, rows[i]+1, W, sz, x, y);
}
*/

static void rndfield(GolState& gol)
{
    u8 *const* const rows = gol.rows;
    const u8 w = gol.w;
    const u8 h = gol.h;
    TinyRng16 rng = gol.rng;
    for(u8 y = 0; y < h; ++y)
    {
        u8 * const row = rows[y];
        for(u8 x = 0; x < w; ++x)
        {
            const u8 b = rng() & 0x55;
            row[x] = b | (b << 1);
        }
    }
    gol.rng = rng;
}

static void musicvis(GolState& gol)
{
#ifdef CFG_ENABLE_AUDIO
    const u8 w = gol.w;
    const u8 h = gol.h;
    u16 y = 1;
    for(u8 i = 0; i < pmfplayer_max_channels && y < h; ++i, y += 4)
    {
        const pmf_channel_info ch = G.mus.player.channel_info(i);
        if(ch.note_hit & 1)
        {
            u8 *row = gol.rows[y];
            const u8 note = ch.base_note & 0x7f;
            u8 lim = (note+7) / 8;
            if(lim > w)
                lim = w;
            for(u8 x = 1; x < lim; ++x)
            {
                row[x] = 0xff;
            }
        }
    }
#endif
}

static void ev_randomize(void *pstate)
{
    GolState& gol = *(GolState*)pstate;
    if(gol.done)
        return;

    gol.randomize = true;
    evs::schedule(8000, ev_randomize, pstate);
}

static void ev_musicvis(void *pstate)
{
    GolState& gol = *(GolState*)pstate;
    if(gol.done)
        return;
    gol.musicvis = true;
    evs::schedule(16, ev_musicvis, pstate);
}

static const Color golpal_PF[] PROGMEM_FAR =
{
    0,
    0xffff,
    LCD::gencolor_inl(0, 0xcf, 0xff),
    LCD::gencolor_inl(0x10,0x20, 0xff)
};


demopart part_gol()
{
    // --- scratchpad setup ---
    static_assert(sizeof(compressbits) == 256, "aa");
    static_assert(sizeof(advancebits) == 256, "aa");

    #define LOADFAR(dst, src, n) fglcd::ProgmemFar::Memcpy(dst, fglcd_get_farptr(src), n)

    LOADFAR(scratch2, compressbits, 256);
    LOADFAR(scratch3, advancebits, 256);

    LOADFAR(ptr_compressedcounts_all_2x, compressedcounts_all_2x, sizeof(compressedcounts_all_2x));
    LOADFAR(ptr_compressedcounts_outer_2x, compressedcounts_outer_2x, sizeof(compressedcounts_outer_2x));
    LOADFAR(ptr_compressedcounts_single, compressedcounts_single, sizeof(compressedcounts_single));
    LOADFAR(ptr_uncompressmask, uncompressmask, sizeof(uncompressmask));
    LOADFAR(ptr_alive, alive, sizeof(alive));

    // --- end scratchpad ---


    auto golpal = farload(golpal_PF);
    applyPal16_RAM(&golpal[0], golpal.N);

    u8 ** const rows = (u8**)alloca(NY * sizeof(u8*));
    u8 * const field = (u8*)alloca(NBUF*2);
    u8 * const zero = (u8*)alloca(NX);
    u8 * const workmem1 = (u8*)alloca(NX);
    u8 * const workmem2 = (u8*)alloca(NX);
    fglcd::RAM::Memset(zero, 0, NX);
    fglcd::RAM::Memset(field, 0, NBUF);
    fglcd::RAM::Memset(workmem1, 0, NX);
    fglcd::RAM::Memset(workmem2, 0, NX);

    {
        u8 *p = &field[1];
        for(u8 y = 1; y <= H; ++y, p += NX)
        {
            FGLCD_ASSERT(p < field+NBUF, "golptr");
            rows[y] = p;
        }
        rows[0] = zero;
        rows[NY-1] = zero;
    }

    GolState gol;
    gol.w = u8(W);
    gol.h = u8(H);
    gol.rows = &rows[1];
    gol.done = false;
    gol.wrk[0] = &workmem1[1];
    gol.wrk[1] = &workmem2[1];
    gol.randomize = false;
    gol.musicvis = false;
    gol.rng.init(1337);

    // field init done

    //ev_randomize(&gol);
    //ev_musicvis(&gol);
    //gol.randomize = true;

    interp::Interpolator<1> inp(16);
    interp::FancyInterpolatedValue<u8> fadeout(0);

    bool exiting = false;
    partdone = false;
    while(true)
    {
        if(!exiting && partdone)
        {
            exiting = true;
            inp.add(fadeout);
            fadeout.interpolateTo(255, 1300);
        }

        if(exiting)
        {
            u8 fade = fadeout;
            if(fade == 255)
                break;
            for(u8 i = 0; i < golpal.N; ++i)
                palsetcolor(i, dampenColor(golpal[i], fade));
            gol.fulldraw = true;
        }

        gol.typer.update();

        /*if(gol.randomize)
        {
            gol.randomize = false;
            rndfield(gol);
        }*/
        /*if(gol.musicvis)
        {
            gol.musicvis = false;
            musicvis(gol);
        }*/
        if(!gol.paused)
        {
            updatefield(gol);
            gol.fulldraw = false;
        }
    }
    gol.done = true;
    partdone = false;
}
