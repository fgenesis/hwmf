#include "demoshared.h"
#include "cbm64.h"
#include "part_c64.h"

//#define DEBUG_SYNC

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif

// WARNING: Oh dang we're out of RAM. Need 2x 1000b for the 40x25 screenbuffer (chars + paths).
// All in all this is not enough with music and everything. So we use the 4x 256b scratch area as path buffer.
// -> The drawpetscii() function in this file requires 4 scratch blocks, so can't use palette data, sin-table, decomp-to-LCD, etc.

//#define VERYFAST

#ifdef VERYFAST
#define WAIT(x)
#else
#define WAIT(x) fglcd::delay_ms(x)
#endif

#define PATH_BUFFER scratch0

#define UNPACK_EXO(dst, src) decompressRAM<PK_EXO, sizeof(dst), fglcd::ProgmemFar>(&dst[0], fglcd_get_farptr(src), sizeof(src));

// 30, 12, 8
enum
{
#ifdef VERYFAST
    DELAY1 = 1,
    DELAY2 = 1,
    DELAY3 = 1,
#else
    DELAY1 = 30,
    DELAY2 = 9,
    DELAY3 = 8,
#endif
    DELAY_CUBE = 8,
    DELAY_CROSS = 4
};

static const uint16_t c64_pal_PF[16] PROGMEM_FAR =
{
0,25356,35921,44405,65535,39528,52206,27298,41799,52912,40723,23883,28152,35833,21043,41652
};

static const u8 c64_bg_blue = 231;
static const Color c64bg = LCD::gencolor_inl(66,66,c64_bg_blue);
static const Color c64fg = LCD::gencolor_inl(165,165,255);


const unsigned C64_W = 320, C64_H = 200;
const unsigned cx0 = LCD::WIDTH/2 - C64_W/2;
const unsigned cy0 = LCD::HEIGHT/2 - C64_H/2;

static void SYNCPOINT()
{
#ifdef DEBUG_SYNC
    LCD::fillrect(AREA_X_BEGIN, AREA_Y_BEGIN, 10, 10, LCD::gencolor_inl(0xff, 0, 0xff));
#endif
    WaitPartDone();
#ifdef DEBUG_SYNC
    LCD::fillrect(AREA_X_BEGIN, AREA_Y_BEGIN, 10, 10, 0);
#endif
}

static NOINLINE void fillchars(const CBM64Font& charset, char c, Color fg, Color bg)
{
    u8 charbuf[40];
    fglcd::RAM::Memset(charbuf, c, 40);
    for(u8 y = 0; y < 25; ++y)
        charset.drawbuf(charbuf, sizeof(charbuf), cx0, cy0+8*y, fg, bg);
}

static NOINLINE void drawborder(Color fg)
{
    Draw::fillrectfromto(AREA_X_BEGIN, AREA_Y_BEGIN, AREA_X_END, cy0-1, fg); // top
    Draw::fillrectfromto(AREA_X_BEGIN, cy0, cx0-1, cy0+C64_H-1, fg); // left
    Draw::fillrectfromto(cx0+C64_W, cy0, AREA_X_END, cy0+C64_H-1, fg); // right
    Draw::fillrectfromto(AREA_X_BEGIN, cy0+C64_H, AREA_X_END, AREA_Y_END, fg); // bottom
}

static void drawmainarea(Color col)
{
    LCD::fillrect(cx0, cy0, C64_W, C64_H, col);
}

static LCD::PixelPos screenpos(u8 sx, u8 sy)
{
    return LCD::PixelPos(cx0 + sx*u16(8), cy0 + sy*u16(8));
}


static FORCEINLINE void drawchar(const CBM64Font& charset, const u8 chr, u8 xchr, u8 ychr, Color fg, Color bg)
{
    LCD::PixelPos pp = screenpos(xchr, ychr);
    LCD::setxywh(pp.x, pp.y, 8, 8);
    charset.drawchar(chr, fg, bg);
}

struct Cursor
{
private:
    u8 sx, sy;
    u8 * const _screen;
    const CBM64Font& _charset;

public:
    volatile u8 fill, done; // toggled every 333 ms
    volatile u8 ign;
    Color fg, bg;

    Cursor(u8 *screen, const CBM64Font& cs, u8 x, u8 y, Color fg_, Color bg_)
        : sx(x), sy(y), _screen(screen), _charset(cs), fill(0), done(0), ign(0), fg(fg_), bg(bg_)
    {
        ev_cursorblink(this);
    }

    ~Cursor()
    {
        // mark for exit and wait until event acknowledged
        done = 1;
        while(done == 1) {} 
    }

    FORCEINLINE LCD::PixelPos screenpos()
    {
        return ::screenpos(sx, sy);
    }

    FORCEINLINE void move(s8 x, s8 y)
    {
        sx += x;
        sy += y;
    }

    void draw()
    {
        const LCD::PixelPos pp = screenpos();
        LCD::fillrect(pp.x, pp.y, 8, 8, fill ? fg : bg);
    }
    void off()
    {
        fill = 0;
        ign = 1;
        draw();
    }
    void enter(u8 c)
    {
        const LCD::PixelPos pp = screenpos();

        _screen[sx + sy*40] = c;
        if(c == '\n')
            LCD::fillrect(pp.x, pp.y, 8, 8, bg);
        else
            drawchar(_charset, c, sx, sy, fg, bg);

        u8 x = sx + 1;
        if(c == '\n' || x >= 40)
        {
            x = 0;
            ++sy;
        }
        sx = x;
        draw();
    }

    static void ev_cursorblink(void *ud)
    {
        Cursor *cur = (Cursor*)ud;
        cur->fill = !cur->fill;

        if(!cur->ign)
        {
            LCD::StateBackup sb;
            cur->draw();
        }

        if(!cur->done)
            evs::schedule(333, ev_cursorblink, ud);
        else
            cur->done = 2;
    }
};



struct TypeIn
{
    Cursor& _cur;
    const u8 *_txtp;
    const u16 delay;
    volatile u8 done;

    typedef int (*Callback)(const TypeIn& ti, int& c);

    Callback cb;

    static int DefaultProc(const TypeIn& ti, int& c)
    {
        return c ? ti.delay : (c = -1);
    }

    TypeIn(Cursor& cur, const u8 *txt, u16 delay)
        : _cur(cur), _txtp(txt), delay(delay), done(0), cb(DefaultProc)
    {
    }

    TypeIn(Cursor& cur, const u8 *txt, u16 delay, Callback cb_)
        : _cur(cur), _txtp(txt), delay(delay), done(0), cb(cb_)
    {
    }

    void start()
    {
        ev(this);
    }

    static void ev(void *ud)
    {
        TypeIn& self = *(TypeIn*)ud;
        int c = fglcd::RAM::read<u8>(self._txtp);
        ++self._txtp;
        int res = self.cb(self, c);
        if(res >= 0)
        {
            if(c >= 0)
                self._cur.enter((u8)c);
            evs::schedule(res, ev, ud);
        }
        else
            self.done = 1;
    }

    void wait() { while(!done) {} }
};

static const u8 ready_PF[] PROGMEM_FAR = "READY.";

static NOINLINE void boot()
{
    partdone = false;
    CBM64Font charset;

    charset.loadCharset2();

    fillchars(charset, 0, 0xffff, 0);
    fglcd::delay_ms(70);

    drawborder(c64fg);

    fillchars(charset, 127, c64fg, c64bg);
    fglcd::delay_ms(60);

    drawmainarea(c64bg);
}

static NOINLINE void loadshit()
{
    partdone = false;
    CBM64Font charset;
    charset.loadCharset2();

/*
"    **** COMMODORE 64 BASIC V2 ****"
" 64k RAM SYSTEM  38911 BASIC BYTES FREE "
""
"READY."
*/

    {
        static const u8 boot_PF[] PROGMEM_FAR = "\n"
                                         "    **** ATMEGA2560 BASIC V2 ****\n\n"
                                         " 8K RAM SYSTEM  345000 LCD BYTES FREE\n\n"
                                         "READY.\n";
        auto boot = farload(boot_PF);
        charset.drawbuf(&boot[0], Countof(boot)-1, cx0, cy0, c64fg, c64bg);
    }

    //DEBUG_RETURN_IF_DONE();

    u8 screen[0x400];
    Cursor cur(screen, charset, 0, 6, c64fg, c64bg);

    //WaitPartDone();

    static const u8 typein1_PF[] PROGMEM_FAR = "LOAD\"RVSN-ONLINE.HEX\",8,1";
    static const u8 loading1_PF[] PROGMEM_FAR = "SEARCHING FOR RVSN-ONLINE.HEX";
    static const u8 loading2_PF[] PROGMEM_FAR = "LOADING";
    static const u8 typein2_PF[] PROGMEM_FAR = "RUN\n";

    {
        {
            auto typein1 = farload(typein1_PF);
            fglcd::delay_ms(2200);
            TypeIn tpi(cur, &typein1[0], 133);
            tpi.start();
            tpi.wait();
            cur.off();
        }
        //DEBUG_RETURN_IF_DONE();

        cur.enter('\n');
        fglcd::delay_ms(100);
        LCD::PixelPos pp = cur.screenpos();
        {
            auto loading1 = farload(loading1_PF);
            charset.drawbuf(&loading1[0], Countof(loading1)-1, pp.x, pp.y, c64fg, c64bg);
        }
        cur.move(0, 3);
        fglcd::delay_ms(300);
        {
            auto loading2 = farload(loading2_PF);
            charset.drawbuf(&loading2[0], Countof(loading2)-1, pp.x, pp.y+8, c64fg, c64bg);
        }
        //WaitPartDone();
        //DEBUG_RETURN_IF_DONE();
        fglcd::delay_ms(1800);
        cur.ign = 0;
        {
            auto ready = farload(ready_PF);
            charset.drawbuf(&ready[0], Countof(ready)-1, pp.x, pp.y+16, c64fg, c64bg);
        }
        //DEBUG_RETURN_IF_DONE();
        fglcd::delay_ms(800);
        //WaitPartDone();

        //DEBUG_RETURN_IF_DONE();
    }

    //cur.enter('\n');
    //fglcd::delay_ms(555);
    //DEBUG_RETURN_IF_DONE();

    {
        //fglcd::delay_ms(1000);
        //DEBUG_RETURN_IF_DONE();
        auto typein2 = farload(typein2_PF);
        TypeIn tpi(cur, &typein2[0], 200);
        tpi.start();
        tpi.wait();
        cur.off();
    }
    //DEBUG_RETURN_IF_DONE();
    fglcd::delay_ms(300);
    //DEBUG_RETURN_IF_DONE();
    // exomizer unpack blinker
    {
        applyPal16_PF(fglcd_get_farptr(c64_pal_PF), Countof(c64_pal_PF));
        TinyRng8 rng(23);
        while(!partdone)
        {
            Color fg = palgetcolor_noinl(rng() & u8(Countof(c64_pal_PF)-1u));
            Color bg = palgetcolor_noinl(rng() & u8(Countof(c64_pal_PF)-1u));
#ifdef MCU_IS_PC
            PalBackup bk;
            fuckpal();
#endif
            drawchar(charset, 0, 39, 24, fg, bg);
            fglcd::delay_ms(60);
            //DEBUG_RETURN_IF_DONE();
        }
        drawchar(charset, 0x20, 39, 24, c64bg, c64bg);
    }
    partdone = false;

#ifdef MCU_IS_PC
    clearpal(0);
#endif
}

enum MoveBit
{
    MV_UP = 1,
    MV_DOWN = 2,
    MV_LEFT = 4,
    MV_RIGHT = 8,
    MV_CROSS_UD = 16,
    MV_CROSS_LR = 32,

    MV_UD = MV_UP | MV_DOWN,
    MV_LR = MV_LEFT | MV_RIGHT,
    MV_MOVE = MV_UD | MV_LR
};

struct Growers;

static inline void addgrower(Growers& grw, u8 x, u8 y, u8 mask);

struct Grower
{
    Grower() {}
    Grower(u8 xx, u8 yy, u8 msk) : x(xx), y(yy), mask(msk) {}

    u8 x, y, mask, remstatus;

    FORCEINLINE void draw(const CBM64Font& charset, const u8 *screen, Color fg, Color bg, bool overdraw) const
    {
        u8 chr;
        if((remstatus & MV_MOVE) && !(remstatus & (MV_CROSS_LR|MV_CROSS_UD)) && (mask & (MV_CROSS_LR|MV_CROSS_UD)))
        {
            if(!overdraw)
                return;
            chr = mask & MV_CROSS_LR ? 67 : 66;
        }
        else
            chr = screen[u16(y)*40+x];
        drawchar(charset, chr, x, y, fg, bg);
    }

    void expand(u8 *status, Growers& grw)
    {
        const u16 idx = u16(y)*40+x;
        const u8 st = mask & status[idx]; // where can we expand to?
        if(st)
        {
            status[idx] &= ~mask; // block further expansion to here, but allow jumping over
            remstatus = status[idx];
            //LCD::setxywh_inl(cx0+u16(x)*8, cy0+u16(y)*8, 1, 1);
            //LCD::sendPixel(LCD::gencolor(0xff, 0, 0));

            if((st & (MV_RIGHT|MV_CROSS_LR)) && x < 39)
            {
                const u8 right = status[idx+1];
                if(right & MV_LEFT)
                    addgrower(grw, x+1, y, MV_MOVE);
                if(x < 38 && (right & MV_CROSS_LR) && (status[idx+2] & MV_LEFT))
                    addgrower(grw, x+1, y, MV_CROSS_LR);
            }

            if((st & (MV_LEFT|MV_CROSS_LR)) && x)
            {
                const u8 left = status[idx-1];
                if(left & MV_RIGHT)
                    addgrower(grw, x-1, y, MV_MOVE);
                if(x > 1 && (left & MV_CROSS_LR) && (status[idx-2] & MV_RIGHT))
                    addgrower(grw, x-1, y, MV_CROSS_LR);
            }

            if((st & (MV_DOWN|MV_CROSS_UD)) && y < 24)
            {
                const u8 down = status[idx+40];
                if(down & MV_UP)
                    addgrower(grw, x, y+1, MV_MOVE);
                if(y < 23 && (down & MV_CROSS_UD) && (status[idx+2*40] & MV_UP))
                    addgrower(grw, x, y+1, MV_CROSS_UD);
            }

            if((st & (MV_UP|MV_CROSS_UD)) && y)
            {
                const u8 up = status[idx-40];
                if(up & MV_DOWN)
                    addgrower(grw, x, y-1, MV_MOVE);
                if(y > 1 && (up & MV_CROSS_UD) && (status[idx-2*40] & MV_DOWN))
                    addgrower(grw, x, y-1, MV_CROSS_UD);
            }
        }
    }
};

struct Growers
{
    Grower g[40]; // prolly enuff
    u8 n;

    void draw(const CBM64Font& charset, const u8 *screen, Color fg, Color bg, bool overdraw) const
    {
        for(u8 i = 0; i < n; ++i)
        {
            g[i].draw(charset, screen, fg, bg, overdraw);
        }
    }

    void expand(u8 *status)
    {
        u8 tail = Countof(g)-n;
        memmove(&g[tail], &g[0], n*sizeof(Grower)); // move memory to end of buffer
        n = 0;
        for(; tail < Countof(g); ++tail)
        {
            g[tail].expand(status, *this);
        }
    }
};

static inline void addgrower(Growers& grw, u8 x, u8 y, u8 mask)
{
    //LCD::setxywh_inl(cx0+u16(x)*8, cy0+u16(y)*8, 1, 1);
    //LCD::sendPixel(LCD::gencolor(0, 0xff, 0));

    grw.g[grw.n++] = Grower(x, y, mask);
}

struct TinyWait
{
    volatile bool done;

    TinyWait(u8 ms)
        : done(false)
    {
        evs::schedule(ms, _ev, this);
    }

    ~TinyWait()
    {
        while(!done) {}
    }

    static void _ev(void *ud)
    {
        TinyWait *self = (TinyWait*)ud;
        self->done = true;
    }
};

static NOINLINE void dogrow_loop(u8 *paths, const CBM64Font& charset, const u8 *screen, Growers& gg, const Color *colors, u8 ncol, u8 delay, Color bg, bool overdraw)
{
    while(gg.n /*&& !partdone*/)
    {
        for(u8 i = 0; i < ncol; ++i)
        {
            TinyWait halt(delay);
            gg.draw(charset, screen, colors[i], bg, overdraw);
        }
        gg.expand(&paths[0]);
    }
}

static NOINLINE void dogrow(const CBM64Font& charset, const u8 *screen, Growers& gg, const Color *colors, u8 ncol, u8 delay, Color bg, bool overdraw)
{
    u8 *paths = PATH_BUFFER;
    UNPACK_EXO(paths, data_exo_paths);

    dogrow_loop(&paths[0], charset, screen, gg, colors, ncol, delay, bg, overdraw);
}

static NOINLINE void drawscreen(const CBM64Font& charset, const u8 *screen, Color fg, Color bg, u8 exclude = 32)
{
    u8 c;
    for(u8 y = 0; y < 25; ++y)
        for(u8 x = 0; x < 40; ++x)
            if((c = *screen++) != exclude)
                drawchar(charset, c, x, y, fg, bg);
}

static const Color burncolors_F[] PROGMEM_FAR =
{
    LCD::gencolor_inl(0xff, 0xdf, 0x5f),
    LCD::gencolor_inl(0xff, 0xbf, 0x2f),
    LCD::gencolor_inl(0xff, 0x8f, 0),
    LCD::gencolor_inl(0xef, 0x3f, 0),
    LCD::gencolor_inl(0x7f, 0x1f, 0),
    LCD::gencolor_inl(0x3f, 0, 0),
    LCD::gencolor_inl(0, 0, 0)
};

static const Color burncolorsFadeIn_F[] PROGMEM_FAR =
{
    LCD::gencolor_inl(0x3f, 0, 0),
    LCD::gencolor_inl(0x7f, 0x1f, 0),
    LCD::gencolor_inl(0xef, 0x3f, 0),
    LCD::gencolor_inl(0xff, 0x8f, 0),
    LCD::gencolor_inl(0xff, 0xbf, 0x2f),
    LCD::gencolor_inl(0xff, 0xdf, 0x5f)
};

static u8 applyBurnColor(Color *colors)
{
    fastmemcpy_PF(colors, burncolors_F, sizeof(burncolors_F));
    return Countof(burncolors_F);
}

static u8 applyBurnColorFadeIn(Color *colors)
{
    fastmemcpy_PF(colors, burncolorsFadeIn_F, sizeof(burncolorsFadeIn_F));
    return Countof(burncolorsFadeIn_F);
}

static NOINLINE void drawpetscii()
{
    CBM64Font charset;
    charset.loadCharset1();

    u8 screen[0x400];

#define TOSCREEN(what) UNPACK_EXO(screen, what)
#define TOPATH(what) UNPACK_EXO(PATH_BUFFER, what)

    TOSCREEN(data_exo_hwmf);
    partdone = false;

    Color colors[8];
    Growers gg;

    // Path appears initially

    colors[0] = c64fg;
    gg.g[0] = Grower(26, 0, MV_MOVE);
    gg.n = 1;
    dogrow(charset, &screen[0], gg, &colors[0], 1, DELAY1, c64bg, true);
    //DEBUG_RETURN_IF_DONE();
    WAIT(1000);
    //SYNCPOINT();

    // "fade out" border and change colors to darker

    drawscreen(charset, &screen[0], c64fg, 0, 0);
    WAIT(1000);
    drawborder(c64bg);
    drawscreen(charset, &screen[0], c64bg, 0, 0);
    WAIT(1000);

    // border goes black

    drawborder(0);
    WAIT(1000);
    //SYNCPOINT();

    for(u8 i = 0; i < 7; ++i)
    {
        u8 c = i*36;
        colors[i] = LCD::gencolor(c, c, c64_bg_blue+c);
    }
    colors[7] = 0xffff;
    gg.g[0] = Grower(4, 0, MV_MOVE);
    gg.g[1] = Grower(11, 0, MV_MOVE);
    gg.n = 2;

    // paint over the lines to full white

    dogrow(charset, &screen[0], gg, &colors[0], Countof(colors), DELAY2, 0, false);
    //DEBUG_RETURN_IF_DONE();
    //SYNCPOINT();

    // Temporarily use the screen memory for the overlay...
    TOSCREEN(data_exo_hwmf_nameoverlay);
    drawscreen(charset, &screen[0], LCD::gencolor_inl(0xff, 0x30, 0), 0);
    TOSCREEN(data_exo_hwmf); // ... and then put the original back

    //DEBUG_RETURN_IF_DONE();
    //WAIT(2500);
    //SYNCPOINT();


    gg.g[0] = Grower(36, 23, MV_MOVE);
    gg.n = 1;

    // last time is special...
    TOPATH(data_exo_paths);
    u8 *p = &PATH_BUFFER[u16(12)*40+18];
    // keep the rhombus in the center unaffected...
    p[0] = 0;
    p[1] = 0;
    p[40] = 0;
    p[41] = 0;

    // erase all the things except the center rhombus

    SYNCPOINT();

    dogrow_loop(&PATH_BUFFER[0], charset, &screen[0], gg, &colors[0], applyBurnColor(colors), DELAY3, 0, false);

    //DEBUG_RETURN_IF_DONE();
    //WAIT(1000);

    // -------------------------------------------------------------
    SYNCPOINT();
    // -------------------------------------------------------------
    // Transition to cube starts here...
    
    TOSCREEN(data_exo_hwmf_cube);
    TOPATH(data_exo_hwmf_path_cube);
    gg.g[0] = Grower(17, 14, MV_MOVE);
    gg.n = 1;
    applyBurnColorFadeIn(colors);
    dogrow_loop(&PATH_BUFFER[0], charset, &screen[0], gg, colors, applyBurnColorFadeIn(colors), DELAY_CUBE, 0, false);

    WAIT(1000);

    TOSCREEN(data_exo_hwmf_cross);
    TOPATH(data_exo_hwmf_path_cross);
    gg.g[0] = Grower(17, 13, MV_MOVE);
    gg.g[1] = Grower(19, 14, MV_MOVE);
    gg.g[2] = Grower(20, 12, MV_MOVE);
    gg.g[3] = Grower(18, 11, MV_MOVE);
    gg.n = 4;
    applyBurnColorFadeIn(colors);
    dogrow_loop(&PATH_BUFFER[0], charset, &screen[0], gg, colors, applyBurnColorFadeIn(colors), DELAY_CROSS, 0, false);

    //fglcd::delay_ms(333);

    // ok this works without clearing properly. and this looks cooler and more sudden imo.

    /*
    TOPATH(hwmf_path_rmcc);
    gg.g[0] = Grower(19, 0, MV_MOVE);
    gg.g[1] = Grower(39, 13, MV_MOVE);
    gg.g[2] = Grower(18, 23, MV_MOVE);
    gg.g[3] = Grower(0, 12, MV_MOVE);
    gg.n = 4;
    applyBurnColorFadeIn(colors);
    dogrow_loop(&PATH_BUFFER[0], charset, &screen[0], gg, colors, applyBurnColor(colors), DELAY_CROSS, 0, false);
    */

    //SYNCPOINT();

    WAIT(500);
}

static NOINLINE void plastic()
{
    DrawImageHelper<data_screenborder_png> img;
    img.applypal();
    img.draw(0, 0);
}

static int _specsCallback(const TypeIn& tpi, int& c)
{
    switch(c)
    {
        default:
            return tpi.delay;
        case '\n':
            return 600;
        case ':':
            return 400;
        case 0xff:
            c = -1;
            return -1;
    }
}

static NOINLINE void specs()
{
    partdone = false;
    drawmainarea(c64bg);

    CBM64Font charset;
    charset.loadCharset1();

    u8 screen[0x400];
    TOSCREEN(data_exo_a2560_title);
    //memset(screen, 128, sizeof(screen));
    TOPATH(data_exo_a2560_path);
    //drawscreen(charset, screen, 0, c64bg);

    Color colors[1];
    Growers gg;

    colors[0] = 0;
    gg.g[0] = Grower(39, 8, MV_LEFT); // 32, 7
    gg.n = 1;
    dogrow_loop(&PATH_BUFFER[0], charset, &screen[0], gg, &colors[0], 1, 18, c64bg, true);

    charset.loadCharset2();
    //DEBUG_RETURN_IF_DONE();
    WAIT(300);

    //for(unsigned i = 0; i < 256; ++i)
    //    drawchar(charset, i, 24+ i % 16, i / 16, 0xffff, 0);


    //0,25356,35921,44405,65535,39528,52206,27298,41799,52912,40723,23883,28152,35833,21043,41652
    Cursor cur(screen, charset, 0, 0, 65535, c64bg);

    // Abuse the path buffer for the type-in text as it's not needed right now
    TOPATH(data_exo_specs);
    TypeIn tpi(cur, &PATH_BUFFER[0], 18, _specsCallback);
    //WaitPartDone();
    tpi.start();
    tpi.wait();
}

#define BAR_HEIGHT 65

static void accumulateAdd(u8vec4& a, const u8vec4& v)
{
    a.x = saturateAdd(a.x, v.x);
    a.y = saturateAdd(a.y, v.y);
    a.z = saturateAdd(a.z, v.z);
    a.w |= v.w;
}

static void addHighlight(u8vec4& a, u8 x)
{
    a.x = saturateAdd(a.x, x);
    a.y = saturateAdd(a.y, x);
    a.z = saturateAdd(a.z, x);
}

static u8vec4 barcolscale(const u8vec4& v, u8 scale)
{
    return u8vec4(
        hiscale8(v.x, scale),
        hiscale8(v.y, scale),
        hiscale8(v.z, scale),
        v.w
    );
}

struct NoiseState
{
    u8 intensity;
};

// TODO MUSICSYNC: change bar width/color on beat (3 channels)
static NOINLINE void noise()
{
    partdone = false;
    loadIsinToScratch2();
    loadUsinToScratch3();
    //applyPal16_PF(fglcd_get_farptr(noisecol), Countof(noisecol));

    //u8 rowflags[LCD::HEIGHT]; // 0: noise only, 1: has bar

    u8 intensity[BAR_HEIGHT];
    {
        constexpr fp1616 step = 128.0f / BAR_HEIGHT;
        fp1616 a = step;
        for(u8 i = 0; i < BAR_HEIGHT-1; ++i, a += step)
        {
            fp1616 m = FSinCosScratch::sin((u8)a.intpart());
            intensity[i] = u8((m * 255).intpart());
        }
        intensity[BAR_HEIGHT-1] = 0; // terminator
    }

    LCD::set_scroll_pos(AREA_X_BEGIN);

    constexpr unsigned areaw =
    LCD::CONST_scroll_area_sides(AREA_X_BEGIN, LCD::XMAX-AREA_X_END);
    LCD::  set_scroll_area_sides(AREA_X_BEGIN, LCD::XMAX-AREA_X_END);
    static_assert(areaw == AREA_W, "areawwtf");

    constexpr static const fp1616 barm_PF[] PROGMEM_FAR = { 0.83f*0.7f, 1.17f*0.7f, 0.61f*0.7f };
    constexpr static const unsigned bary_PF[] PROGMEM_FAR = { 2*(LCD::WIDTH/5), 3*(LCD::WIDTH/5), 4*(LCD::WIDTH/5) };
    constexpr static const u8 barcol_PF[] PROGMEM_FAR = {
        0xaf, 0, 0, 1,
        0, 0x96, 0, 2,
        0x17, 0x30, 0xdf, 4
    };

    auto barm = farload(barm_PF);
    auto bary = farload(bary_PF);
    auto barcol = (u8vec4*)(farload(barcol_PF).ptr());

    const u16 INTERLACE = 8;
    const u16 INTERLACESTEP = 3;
    u16 yinterlace = AREA_Y_BEGIN;
    //u16 v = 0;
    //u16 vv = 0;
    u16 a = 0;

    u8vec4 linebuf[AREA_H];
    TinyRng8 rng(0x37), rng2(0x91), rng3(0x63); // faster than one 16 bit rng

    interp::Interpolator<3> inp(16);
    MusicSyncVar<u8> beatsync[] =
    {
        MusicSyncVar<u8> (inp, BEATSYNC_CHANNEL1, 0, 55, 3333),
        MusicSyncVar<u8> (inp, BEATSYNC_CHANNEL2, 0, 70, 3333),
        MusicSyncVar<u8> (inp, BEATSYNC_CHANNEL3, 0, 40, 3333)
    };

    while(!partdone)
    {
        fglcd::RAM::Memset(&linebuf[0], 0, sizeof(linebuf));

        for(u8 i = 0; i < Countof(bary); ++i)
        {
            fp1616 s = FSinCosScratch::sin((u8)(barm[i] * a).intpart());
            int y = (AREA_H/2) + (s * ((AREA_H/2)-(BAR_HEIGHT/2))).intpart();
            bary[i] = y;

            // accumulateAdd colors
            const u8vec4 bc = barcol[i];
            const unsigned mi = vmax<int>(0,      y - BAR_HEIGHT/2);
            const unsigned ma = vmin<int>(AREA_H, y + BAR_HEIGHT/2);
            const u8 d = ma - mi;
            const u8 beat = beatsync[i];
            for(u8 k = 0; k < d; ++k)
            {
                u8vec4& v = linebuf[mi+k];
                accumulateAdd(v, barcolscale(bc, intensity[k]));
                if(beat)
                    addHighlight(v, beat); 
            }
        }

        /*for(u8 i = 0; i < Countof(bary); ++i)
        {
            unsigned y = AREA_Y_BEGIN + (AREA_H/2) + bary[i];
            Draw::fillrectfromto(AREA_X_BEGIN, y, AREA_X_END, y, 0xffff);
        }*/


        // DRAW LINES NOW

        const u8 xxr = rng3();

        yinterlace = (yinterlace + INTERLACESTEP) % INTERLACE;
        for(u16 YBASE = yinterlace; YBASE < AREA_H; YBASE += INTERLACE /*, ++vv*/)
        {
            const unsigned Y = AREA_Y_BEGIN + YBASE;
            const u8vec4 linecol = linebuf[YBASE];
            //const Color basecol = LCD::gencolor_inl(linecol.x, linecol.y, linecol.z);
            //Draw::fillrectfromto(AREA_X_BEGIN, Y, AREA_X_END, Y, basecol);

            LCD::setxy(AREA_X_BEGIN, Y, AREA_X_END, Y);

            const Color col1 = LCD::gencolor_inl(linecol.x, linecol.y, linecol.z);
            if(linecol.w)
            {
                LCD::setColor(col1);
                LCD::fastfill_u16(AREA_W);
                rng(); // this prevents artifacts in the bg noise
            }
            else
            {
                const u8 s = USIN8FAST(u8(a+YBASE)*8);
                const u8 PLUS = saturateAdd<u8>(s, 0x9f);
                const Color col0  = LCD::gencolor_inl(PLUS, PLUS, PLUS);
            
                /*LCD::gencolor_inl(
                    saturateAdd(linecol.x, PLUS),
                    saturateAdd(linecol.y, PLUS),
                    saturateAdd(linecol.z, PLUS)
                );*/


                //v += vv;
                const u8 xr = xxr ^ rng2();
                FGLCD_DUFF8(u16, areaw, {
                    //v += (v << 1) + 42;
                    //v ^= (v >> 3) ^ (v << 3);
                    //LCD::sendPixel(palgetcolor_inl(v & 1));
                    const u8 b = xr ^ rng();
                    //v += b;
                    LCD::sendPixel( (b & 1) ? col1 : col0);
                });
                //vv = v;
            }
            u16 scroll = u16(rng()) + rng();
            scroll = vmodpos<u16, areaw>(scroll);
            LCD::set_scroll_pos(AREA_X_BEGIN + scroll);
        }

        ++a;
  }
  LCD::set_scroll_area_sides(0, 0);
  LCD::set_scroll_pos(0);
  //LCD::set_refresh_rate(LCD::HZ_125);
}

// -------------------------------------- PLASMA ---------------------------------

static NOINLINE Color hsvcolor(u16 h, u8 s, u8 v)
{
    u8 r,g,b;
    fast_hsv2rgb_8bit(h, s, v, &r, &g, &b);
    return LCD::gencolor(r,g,b);
}

// red gradient and solid green
static void palcolor_rg(u16 *pal)
{
    for(u8 i = 0; i < 128; ++i)
    {
        pal[i] = LCD::gencolor(i*2, 0,0);
        pal[i+128] = LCD::gencolor(0,i < 80 ? 0 : 0xff,i);
    }
}
static void palcolor_mush(u16 *pal)
{
    const u8 split = 19;
    u8 i = 0, x = 0;
    Color c1 = LCD::gencolor_inl(20, 50, 211);
    Color c2 = LCD::gencolor_inl(20, 40, 0);
    for(;;)
    {
        Color c = x ? c1 : c2;
        for(u8 k = 0; k < split; ++k)
        {
            pal[i++] = c;
            if(!i)
                return;
        }
        x = 1 - x;
    }
}

static void palcolor_rainbow(u16 *pal)
{
    u8 i = 0;
    do
        pal[i] = hsvcolor(i * (HSV_HUE_MAX / 256), 255, 255);
    while(++i);
}

static void palcolor_fire(u16 *pal)
{
    u8 i = 0;
    do
        pal[i] = hsvcolor(0xff - i, 255, 255);
    while(++i);
}

// [white][ ------------------ black --------------------]
static void palcolor_mostblack(u16 *pal)
{
    const u16 d = 40;
    fglcd::RAM::Memset(pal, 0, 512);
    for(u8 i = 0; i < d; ++i)
        pal[i] = 0xffff;
}

// [ ------------------ white --------------------][black]
static void palcolor_mostwhite(u16 *pal)
{
    const u16 d = 40;
    fglcd::RAM::Memset(pal, 0xff, 512);
    for(u16 i = 256-d; i < 256; ++i)
        pal[i] = 0;
}

static void palcolor_cold(u16 *pal)
{
    u8 i = 0;
    do
        pal[i] = hsvcolor(127*8-(i/2), 255, i);
    while(++i);
}

static void palcolor_green(u16 *pal)
{
    u8 i = 0;
    do
    {
        u16 x = (17 * i) >> 3;
        pal[i] = hsvcolor(127*2+x, 225-(i>>2), 50+i);
    }
    while(++i);
}

static void palcolor_c64(u16 *pal)
{
    auto pp = farload(c64_pal_PF);
    u8 i = 0;
    for(u8 j = 0; j < Countof(pp); ++j)
    {
        Color c = pp[j];
        for(u8 k = 0; k < 256 / Countof(pp); ++k)
            pal[i++] = c;
    }
}

typedef void (*ColorFunc)(u16*);
static const ColorFunc plasmacolfunc_PF[] PROGMEM_FAR =
{
    /* 0 */ palcolor_rg,
    /* 1 */ palcolor_mush,
    /* 2 */ palcolor_rainbow,
    /* 3 */ palcolor_fire,
    /* 4 */ palcolor_mostblack,
    /* 5 */ palcolor_mostwhite,
    /* 6 */ palcolor_cold,
    /* 7 */ palcolor_green,
    /* 8 */ palcolor_c64,
};

// TODO: color scheme
struct PlasmaParams
{
    u8 interlace;
    u8 interlacestep;
    u8 speed;
};

static const PlasmaParams coolparams_PF[] PROGMEM_FAR =
{
    /* 0 */ { 2, 2, 2 }, // can overlay this over a bg picture?
    /* 1 */ { 1, 1, 1 }, // TEST THIS
    /* 2 */ { 5, 2, 2 }, // really nice
    /* 3 */ { 13, 5, 2 },
    /* 4 */ { 13, 11, 2},
    /* 5 */ { 17, 7, 2 },
};

struct PlasmaState
{
    PlasmaState() : rng(36568) {} // 34768

    volatile u8 palchanged;
    volatile u8 paramid;
    volatile u8 funcid;
    volatile s8 xinc; // -2 .. +2
    TinyRng16 rng;
    u16 pal[256];
};

static void modplasma(PlasmaState *ps, u8 mod)
{
    u8 p = ps->paramid, f = ps->funcid;
    const u8 oldf = f;
    s8 xi = ps->xinc;
    u8 negxi = xi < 0;
    xi = vabs(xi);
    if(mod & 1)
        --p;
    if(mod & 2)
        ++p;
    if(mod & 4)
        --f;
    if(mod & 8)
        ++f;
    if(mod & 16)
        xi = xi == 1 ? 2 : 1;
    if(mod & 32)
        negxi = !negxi;

    if(negxi)
        xi = -xi;
    ps->xinc = xi;

    p = vmodpos<s8, Countof(coolparams_PF)>(p);
    ps->paramid = p;

    if(f != oldf)
    {
        f = vmodpos<s8, Countof(plasmacolfunc_PF)>(f);
        ps->funcid = f;
        ps->palchanged = 1;
    }
}

static void plasmasync(uint8_t channel, pmf_channel_info info, void *ud)
{
    PlasmaState *ps = (PlasmaState*)ud;
    u8 r;
    do
    {
        u16 rr  = ps->rng();
        r = (u8)rr;
        r &= ~(2 | 4 | 8); // only forward, never back
    }
    while(!r); // make sure *something* is changed everytime
    r |= 8; // always jump the colorscheme
    modplasma(ps, r);
}

static void dbg_plasma(DebugThing& dbg, void *ud, u8 b, u8 bx)
{
    PlasmaState *ps = (PlasmaState*)ud;
    if(bx)
        modplasma(ps, b & bx);
    dbg.set2Num(ps->paramid, ps->funcid);
}

// TODO MUSICSYNC: change pattern/color on beat?
static NOINLINE void plasma()
{
    PlasmaState ps;
    ps.funcid = 0;
    ps.paramid = 0;
    ps.xinc = 2;
    ps.palchanged = 0;

    const auto allparams = farload(coolparams_PF);

    partdone = false;
    loadIsinToScratch2();
    loadUsinToScratch3();

    u16 colorbuf[256];

    palcolor_green(colorbuf);
    applyPal16_RAM(colorbuf, 256);

    u8 yinterlace = 0;
    u16 t = 0;
    u16 sp = 0;

    DEBUG_THING(dbg_plasma, &ps);

    musync::Override mu(BEATSYNC_CHANNEL1, plasmasync, &ps, musync::CallOnHit);

    while(!partdone)
    {
        const PlasmaParams pp = allparams[ps.paramid];
        if(ps.palchanged)
        {
            ps.palchanged = 0;
            void *colf = fglcd::ProgmemFar::read<void*>(fglcd_get_farptr(plasmacolfunc_PF), ps.funcid);
            ((ColorFunc)colf)(colorbuf);
            applyPal16_RAM(colorbuf, 256);
        }

        t += pp.speed;
        const u16 ysep = LCD::HEIGHT / 2; //(LCD::HEIGHT / 2) + (UCOS8(t)) - 127;
    
        yinterlace += pp.interlacestep;
        yinterlace = vmodpos(yinterlace, pp.interlace);
        const u8 xi = ps.xinc;
        for(u16 YBASE = yinterlace; YBASE < AREA_H; YBASE += pp.interlace, ++sp)
        {
          const u16 yline = AREA_Y_BEGIN + YBASE;
          LCD::setxy(AREA_X_BEGIN, yline, AREA_X_END, yline);
    
          const u16 Y = YBASE + (UCOS8FAST(t) >> 3u);
          
          u8 x = t >> 2;
          const u16 y = Y+t;
          const u8 y1 = y >> 1u;
          const u8 yy2 = y + y1;
          const u8 y5 = (y / 5u);
          const u8 tmpy = y1 + USIN8FAST(y5);
          
          FGLCD_DUFF4(u8, AREA_W/3, {
             u8 v = USIN8FAST(x + t);
             //const u8 vx = v;
             v += tmpy;
             //const u8 v2 = UCOS8(x+USIN8(vx)+y);
             v += USIN8FAST(USIN8FAST(x+yy2) >> 1u);
             //v += USIN8(y + UCOS8(x >> (YBASE&1))); // old red plasma: v += USIN8(y + UCOS8(x));
             v += USIN8FAST(ulo8(y) + UCOS8FAST(x));
    
             LCD::sendPixel(palgetcolor_inl(v));
             LCD::sendPixel();
             LCD::sendPixel();
             x += xi;
          });
        }
    }
}

static NOINLINE void guru()
{
    partdone = false;

    DrawFont<data_topaz_glf> font(scratch3);

    const u8 THICK = 3;
    const u8 PAD = 4;

    const typename LCD::PixelPos begin(AREA_X_BEGIN+PAD+THICK + 5, AREA_Y_BEGIN+PAD+THICK + 30);
    typename LCD::PixelPos pp = begin;
    //font.template drawStr<fglcd::ProgmemFar>(pp, fglcd_get_farptr(blah), 0xffff, LCD::gencolor_inl(0,0,0x3f));
    pp = font.template drawStr<fglcd::ProgmemFar>(pp, fglcd_get_farptr(data_raw_guru), LCD::gencolor_inl(255,0,0), 0);

    const Color red = LCD::gencolor_inl(255,0,0);

    //DoNextPartIn(2600);
    for(u8 i = 0; !partdone; ++i)
    {
        Draw::borderrectfromto(begin.x-PAD-THICK, begin.y-PAD-THICK,
            AREA_X_END-PAD, pp.y + PAD,
            (i & 4) ? 0 : red, THICK);
        WAIT(125);
    }


}

static NOINLINE void readyready()
{
    partdone = false;
    CBM64Font charset;
    charset.loadCharset2();

    u8 screen[0x400];
    u8 *p = PATH_BUFFER;
    for(u8 y = 0; y < 25; ++y)
    {
        fastmemcpy_PF(p, ready_PF, Countof(ready_PF)-1); // skip \0
        p += Countof(ready_PF)-1;
        *p++ = '\n';
    }
    p[-1] = 0;

    Cursor cur(screen, charset, 0, 0, c64fg, c64bg);
    cur.off();
    TypeIn tpi(cur, PATH_BUFFER, 1);
    tpi.start();
    tpi.wait();
    WAIT(1022);

    TinyRng16 rng(44);

#ifdef MCU_IS_PC
    PalBackup bk;
    fuckpal();
#endif

    //DoNextPartIn(444);
    while(!partdone)
    {
        for(u8 y = 0; y < 25 && !partdone; ++y)
        {
            for(u8 x = 0; x < 40; ++x)
                drawchar(charset, (u8)rng(), x, y, c64fg, c64bg);
            WAIT(20);
        }
    }
}

static NOINLINE void fillarea(Color col)
{
    Draw::fillrectfromto(AREA_X_BEGIN, AREA_Y_BEGIN, AREA_X_END, AREA_Y_END, col);
}

void warmup()
{
    for(u8 i = 0; i < 50; i += 4)
    {
        fillarea(LCD::gencolor(i,i,i));
        WAIT(20);
    }
}

demopart part_c64()
{
    plastic(); evs::killAllEvents();
    //WAIT(300);
    warmup(); evs::killAllEvents();
    boot(); evs::killAllEvents();
    //WAIT(300);
    loadshit(); evs::killAllEvents();
    //WAIT(300);
    specs(); evs::killAllEvents();
    part_layerchess(); evs::killAllEvents();
    fillarea(0);
    part_sierpscroll(); evs::killAllEvents();
    noise(); evs::killAllEvents();
    plasma(); evs::killAllEvents();
    fillarea(0);
    guru(); evs::killAllEvents();
    fillarea(0);
    WAIT(50);
    boot(); evs::killAllEvents();
    WAIT(100);
    readyready(); evs::killAllEvents();
    fillarea(0);
    WAIT(50);
    boot(); evs::killAllEvents();
    WAIT(200);
    drawpetscii(); evs::killAllEvents();
}
