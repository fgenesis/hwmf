#pragma once

#include "demo_def.h"
#include "fontdef.h"
#include "drawhelper.h"
#include "decomp.h"

namespace drawfont {

// Minimal API required to draw a font
// Instead of this we could use 'LCD' directly, but this indirection is to catch errors
struct DrawToLCD
{
    typedef LCD::ColorType ColorType;
    typedef LCD::DimType DimType;

    static const DimType WIDTH = LCD::WIDTH;

    FORCEINLINE static void setColor(Color col)
    {
        LCD::setColor(col);
    }

    FORCEINLINE static void fastfill_u8(uint8_t n)
    {
        LCD::fastfill_u8(n);
    }

    FORCEINLINE static void setxywh(DimType x, DimType y, DimType w, DimType h)
    {
        LCD::setxywh_inl(x, y, w, h);
    }
};


template<typename Reader, typename Out>
FORCEINLINE static void drawLZ4count(Out& out, typename Reader::Pointer& src, uint8_t n, typename Out::ColorType col)
{
    out.setColor(col);
    out.fastfill_u8(n);
    if(n == 0xf)
    {
        do
        {
            n = Reader::readIncFast(src);
            out.fastfill_u8(n);
        }
        while(n == 0xff);
    }
}

// just emit pixels; assumes that the drawing rect is already set up
template<typename Reader, typename Out>
FORCEINLINE static void draw_1bpp_lz4counts_npix_inl(Out& out, typename Reader::Pointer src, typename Out::ColorType fg, typename Out::ColorType bg)
{
    typename Reader::SegmentBackup seg;
    Reader::SetSegmentPtr(src);
    for(uint8_t c; (c = Reader::readIncFast(src)); ) // zero byte marks end of run
    {
        drawLZ4count<Reader, Out>(out, src, c & 0xf, bg);
        drawLZ4count<Reader, Out>(out, src, c >> 4u, fg);
    }
}

struct FontDef
{
    uint8_t *index;
    uint16_t *offp;
    typename fglcd::RAM::Pointer data;
    uint8_t height;
};

struct CharDef
{
    uint8_t w, h;
    const uint8_t *datastart;
};


CharDef getCharDef(uint8_t c, const FontDef& fnt);

// Assumes that font data have been decompressed to RAM
// Modifies pp
template<typename Out>
static void drawchar(Out& out, typename LCD::PixelPos& pp, const char c, typename Out::ColorType fg, typename Out::ColorType bg, const FontDef& fnt, typename Out::DimType startx)
{
    FGLCD_ASSERT(c != 13, "dos2unix"); // make SURE to have UNIX file endings in text files!!
    if(LIKELY(c != '\n'))
    {
        CharDef cdef = getCharDef(c, fnt);
        if(UNLIKELY(pp.x+cdef.w >= LCD::WIDTH))
            goto newline;
        out.setxywh(pp.x, pp.y, cdef.w, cdef.h);
        pp.x += cdef.w;
        drawfont::draw_1bpp_lz4counts_npix_inl<fglcd::RAM, Out>(out, cdef.datastart, fg, bg);
    }
    else
    {
newline:
        pp.y += fnt.height;
        pp.x = startx;
    }
}

void metrics(LCD::PixelPos& pp, const char c, const FontDef& fnt, const LCD::DimType maxwidth);

template<typename ReadStr, typename Out>
static NOINLINE typename LCD::PixelPos drawfontloop_noinl(
  Out& out,
  typename LCD::PixelPos pp,
  typename ReadStr::Pointer str, typename Out::ColorType fg, typename Out::ColorType bg,
  const FontDef& fnt
) {
    typename ReadStr::SegmentBackup seg;
    const typename Out::DimType startx = pp.x;
    for(char c; (c = ReadStr::readIncSafe(str)); )
        drawchar<Out>(out, pp, c, fg, bg, fnt, startx); // modifies pp
    return pp;
}

template<typename ReadStr>
static NOINLINE LCD::PixelPos calcfontsize_noinl(
  typename ReadStr::Pointer str,
  const FontDef& fnt,
  LCD::DimType maxwidth
) {
    typename ReadStr::SegmentBackup seg;
    LCD::PixelPos pp;
    pp.x = 0;
    pp.y = 0;
    LCD::DimType xmax = 0;
    uint8_t prev = 0;
    for(char c; (c = ReadStr::readIncSafe(str)); )
    {
        metrics(pp, c, fnt, maxwidth); // modifies pp
        xmax = vmax(xmax, pp.x);
    }
    if(!pp.y)
        pp.y = fnt.height;
    return LCD::PixelPos(xmax, pp.y);
}

void buildindex_PF(unsigned char *index, fglcd::FarPtr rd, uint8_t n);

} // end namespace drawfont


// Unpack Font data to RAM. Needs an external index array of exactly 256 bytes.
template<typename TFont>
struct DecompFontData : _DecompDataStorage<TFont::packtype, TFont>
{
    //static_assert(!TFont::blocked, "blocked font does not make sense");
    typedef _DecompDataStorage<TFont::packtype, TFont> Storage;
    typedef typename Storage::Mem Mem;
    uint16_t ramoffs[TFont::noffsets]; // way better to keep this in RAM than in progmem
    DecompFontData(uint8_t *index)
    {
        this->_prepare();
        fastmemcpy_PF(ramoffs, TFont::offsets, sizeof(ramoffs)); // HMM: if sizeof(void*) == sizeof(ramoffs[0]), we could do dirty ptr aliasing + offset delta-encoding, saving some more space, probably
        buildIndex(index);
    }
    void buildIndex(uint8_t *index)
    {
        fglcd::ProgmemFar::Pointer rd = fglcd_get_farptr(TFont::usedch);
        drawfont::buildindex_PF(index, rd, TFont::nusedch);
    }
};

template<typename TFont, typename Out_ = drawfont::DrawToLCD>
struct DrawFont : public DecompFontData<TFont>
{
    typedef DecompFontData<TFont> Base;
    typedef Out_ Out;
    typedef typename LCD::PixelPos PP;
    typedef typename Out::ColorType Color;

    Out *_out;

    // index must be exactly 256 bytes large
    FORCEINLINE DrawFont(unsigned char *index, Out *out = 0)
        : Base(index)
        , _out(out)
    {
        fnt.index = index;
        fnt.offp = &this->ramoffs[0];
        fnt.height = TFont::fontheight;
        fnt.data = this->ptr();
    }

    FORCEINLINE void rebuildIndex() { this->buildIndex(fnt.index); }

    FORCEINLINE uint8_t lineHeight() const { return TFont::fontheight; }

    template<typename Mem>
    FORCEINLINE PP drawStr(PP pp, typename Mem::VoidPointer ptr, Color fg, Color bg) const
    {
        typename Mem::Pointer str = (typename Mem::Pointer)ptr;
        return drawfont::drawfontloop_noinl<Mem, Out>(*_out, pp, str, fg, bg, fnt);
    }
    FORCEINLINE PP drawChar(PP pp, char c, Color fg, Color bg, typename Out::DimType startx) const
    {
        drawfont::drawchar<Out>(*_out, pp, c, fg, bg, fnt, startx); // modifies pp
        return pp;
    }

    // bounding box of text, considers \n to start new lines and
    // retains the maximum x extent seen on a line
    template<typename Mem>
    FORCEINLINE PP calcSize(typename Mem::VoidPointer ptr) const
    {
        typename Mem::Pointer str = (typename Mem::Pointer)ptr;
        return drawfont::calcfontsize_noinl<Mem>(str, fnt, Out::WIDTH);
    }

private:
    drawfont::FontDef fnt;
};

struct FontTyperBase
{
    // callback can modify the char to be printed. set c=0 to not print char.
    // return time to wait (in ms) until next char. return 0 to stop typing.
    typedef uint16_t (*Callback)(char& c, LCD::PixelPos pp, void *ud);

    // modify pixel pos to where to print next
    typedef void (*DrawFunc)(char c, LCD::PixelPos& pp, void *ud);

    Callback cb;
    void *udcb;
    DrawFunc draw;
    void *uddraw;
    LCD::PixelPos pos;
    LCD::DimType startx;
    const char *text; // modifies text in place!

    FontTyperBase(const char *text_, Callback cb_, void *udcb_, DrawFunc draw_, void *uddraw_);
    ~FontTyperBase();
    void start(uint16_t delay); // start in background; eventsystem will do the drawing
    void update(); // ONLY call this when bg==false was passed to ctor. returns true when something was printed
    static void ev_tick(void *p);
    void schedule(uint16_t ms);

    FORCEINLINE void setPos(LCD::DimType x, LCD::DimType y)
    {
        pos.x = x;
        pos.y = y;
        startx = x;
    }

    FORCEINLINE uint8_t done() const
    {
        return _done;
    }

private:
    void _tick();
    volatile uint8_t _done, _haschar;
    volatile char _nextchar;
    uint16_t _enter(char c);
    void _enterAndSchedule(char c);
};

template<typename DF> // DrawFont
struct FontTyper : public FontTyperBase
{
    typedef FontTyper<DF> Self;
    typedef typename DF::Out Out;
    typedef typename DF::Color Color;
    typedef typename DF::PP PP;

    const DF& df;
    Color fgcol, bgcol; // set this after ctor

    static void s_draw(char c, PP& pp, void *ud)
    {
        Self *self = (Self*)ud;
        pp = self->df.drawChar(pp, c, self->fgcol, self->bgcol, self->startx);
    }

    FontTyper(const DF& df_, const char *text_, Callback cb_, void *udcb_)
        : FontTyperBase(text_, cb_, udcb_, s_draw, this), df(df_), fgcol(Color(-1)), bgcol(0)
    {
    }
};
