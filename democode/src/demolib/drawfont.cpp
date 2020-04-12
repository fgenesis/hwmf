#include "drawfont.h"

namespace drawfont {

CharDef getCharDef(uint8_t c, const FontDef& fnt)
{
    const uint8_t idx = fnt.index[c];
    // If this assert fails, an attempt was made to print a glyph that doesn't exist in the font,
    // likely optimized away by the packer. Regenerate font files properly!
    FGLCD_ASSERT(idx != 0xff, "fontidx");
    const uint16_t offs = fnt.offp[idx];
    typename fglcd::RAM::Pointer p = fnt.data + offs;
    const uint8_t w = fglcd::RAM::readIncFast(p);
    const uint8_t h = fglcd::RAM::readIncFast(p);
    const CharDef cdef { w, h, p };
    return cdef;
}

void metrics(LCD::PixelPos& pp, const char c, const FontDef& fnt, const LCD::DimType maxwidth)
{
    FGLCD_ASSERT(c != 13, "dos2unix"); // make SURE to have UNIX file endings in text files!!
    if(LIKELY(c != '\n'))
    {
        CharDef cdef = getCharDef(c, fnt);
        if(UNLIKELY(pp.x+cdef.w >= maxwidth))
            goto newline;
        pp.x += cdef.w;
    }
    else
    {
        newline:
        pp.y += fnt.height;
        pp.x = 0;
    }
}

void buildindex_PF(unsigned char *index, fglcd::FarPtr rd, uint8_t n)
{
    typename fglcd::ProgmemFar::SegmentBackup seg;
#ifdef _DEBUG
    fglcd::RAM::Memset(index, 0xff, 256);
#endif
    fglcd::ProgmemFar::SetSegmentPtr(rd);
    for(uint8_t i = 0; i < n; ++i)
    {
        const uint8_t p = fglcd::ProgmemFar::readIncFast(rd);
        index[p] = i;
    }
}


} // end namespace drawfont


FontTyperBase::FontTyperBase(const char *text_, Callback cb_, void *udcb_, DrawFunc draw_, void *uddraw_)
    : cb(cb_), udcb(udcb_), draw(draw_), uddraw(uddraw_), text(text_)
    , _done(0), _haschar(0), _nextchar(0)
{
}

FontTyperBase::~FontTyperBase()
{
    if(_done != 1)
    {
        _done = 2;
        while(_done != 1) {}
    }
}

void FontTyperBase::start(uint16_t delay)
{
    if(delay)
        schedule(delay);
    else
        _tick();
}


void FontTyperBase::ev_tick(void *p)
{
    ((FontTyperBase*)p)->_tick();
}

void FontTyperBase::schedule(uint16_t ms)
{
    if(!_done)
        evs::schedule(ms, ev_tick, this);
}

void FontTyperBase::_tick()
{
    if(_done)
    {
        _done = 1;
        return;
    }
    _nextchar = *text++;
    _haschar = 1;
}

void FontTyperBase::_enterAndSchedule(char c)
{
    if(uint16_t t = _enter(c))
        schedule(t);
    else
        _done = 1;
}

uint16_t FontTyperBase::_enter(char c)
{
    uint16_t t = cb(c, pos, udcb);
    if(c)
        draw(c, pos, uddraw);
    return t;
}

void FontTyperBase::update()
{
    if(_haschar)
    {
        _haschar = 0;
        _enterAndSchedule(_nextchar);
    }

}