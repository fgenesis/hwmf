#include "demoshared.h"

#ifdef __GNUC__
#pragma GCC optimize ("Os")
#endif

demopart part_thissideup()
{
    DrawImageHelper<data_sideup_png> img;
    img.applypal();

    partdone = false;
    for(u8 y = LCD::HEIGHT/3; y > 10 && !partdone; y -= 1)
    {
        img.draw(LCD::WIDTH/2 - img.w/2, y);
        LCD::fillrect(LCD::WIDTH/2 - img.w/2, y+img.h, img.w, 1, 0); 
        fglcd::delay_ms(20);
        if(y < 10+64)
        {
            u8 c = (y - 10)*3;
            palsetcolor(1, LCD::gencolor(c,c,c));
        }
    }

    cls();
}



typedef DrawFont<data_vga_glf> TheFont;
typedef FontTyper<TheFont> TypeText;

#define UNPACK_EXO(dst, src) decompressRAM<PK_EXO, sizeof(dst), fglcd::ProgmemFar>(&dst[0], fglcd_get_farptr(src), sizeof(src));

static uint16_t cb1(char& c, LCD::PixelPos pp, void *ud)
{
    return c ? 70 : 0;
}

demopart part_intro1()
{
    partdone = false;
    TheFont font(scratch3);
    auto text = farload(data_raw_intro1);
    TypeText typer(font, (const char*)&text[0], cb1, NULL);
    LCD::PixelPos sz = font.calcSize<fglcd::RAM>((const char*)&text[0]);
    typer.setPos(LCD::WIDTH/2 - sz.x/2, LCD::HEIGHT/2 - sz.y/2);
    typer.start(10);

    while(!typer.done())
        typer.update();

    WaitPartDone();
}


struct Intro2State
{
    TypeText typer;
    uint8_t state;
    LCD::PixelPos pos;

    static uint16_t cb(char& c, LCD::PixelPos pp, void *ud)
    {
        if(!c)
            return 0;
        if(c == '\n')
        {
            ((Intro2State*)ud)->next();
            c = 0;
            return 600;
        }
        return 20;
    }

    Intro2State(const TheFont& font, const char *text)
        : typer(font, text, cb, this), state(0)
    {
    }

    void setPos(LCD::DimType x, LCD::DimType y)
    {
        typer.setPos(x, y);
        pos.x = x;
        pos.y = y;
    }

    void next()
    {
        ++state;
        setcolor();
        LCD::PixelPos p = pos;
        p.x += 3;
        p.y += 4;
        setPos(p.x, p.y);
    }

    void setcolor()
    {
        u8 c = 120 + state * 30;
        u8 i = 20 + 10*state;
        typer.fgcol = LCD::gencolor(c,c,c);
        typer.bgcol = LCD::gencolor(i,i,i);
    }
};

demopart part_intro2()
{
    partdone = false;
    char text[512];
    UNPACK_EXO(text, data_exo_intro2);
    TheFont font(scratch3);
    
    Intro2State state(font, text);

    LCD::PixelPos sz = font.calcSize<fglcd::RAM>((const char*)&text[0]);
    state.setPos(LCD::WIDTH/2 - sz.x/2, LCD::HEIGHT/2 - font.lineHeight()/2);
    state.typer.start(10);
    while(!state.typer.done())
        state.typer.update();

    WaitPartDone();
}

demopart part_end()
{
    char text[512];
    UNPACK_EXO(text, data_exo_theend);

    LCD::PixelPos pp;
    {
        TheFont font(scratch3);
        LCD::PixelPos sz = font.calcSize<fglcd::RAM>((const char*)&text[0]);
        pp.x = LCD::WIDTH/2 - sz.x/2;
        pp.y = LCD::HEIGHT/2 - sz.y/2 - data_github_png::h/2;
        pp = font.drawStr<fglcd::RAM>(pp, &text[0], 0xffff, 0);
    }

    {
        DrawImageHelper<data_github_png> img;
        img.applypal();
        img.draw(LCD::WIDTH/2 - img.w/2, pp.y + img.h);
    }

    DoNextPartIn(12000);
    WaitPartDone();
    music_stop();
    cls();

    DoNextPartIn(2000);
    WaitPartDone();
}
