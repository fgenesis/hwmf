#pragma once

// See http://overskill.alexshu.com/128x64-12864-lcd-w-arduino-serial/ for wiring


// Define exactly one of these before including
//#define FGTERM_USE_128x64
//#define FGTERM_USE_SERIAL
//#define FGTERM_USE_2004

#ifdef FGTERM_USE_2004
    #include <Wire.h>
    #include <LiquidCrystal_PCF8574.h>
#elif defined(FGTERM_USE_128x64)
    #include "U8glib.h"
    /*#include <U8g2lib.h>
    #ifdef U8X8_HAVE_HW_SPI
    #include <SPI.h>
    #endif
    #ifdef U8X8_HAVE_HW_I2C
    #include <Wire.h>
    #endif*/
#else
# error Which LCD?
#endif

namespace fgterm {

#ifdef FGTERM_USE_2004
    #include <LiquidCrystal_PCF8574.h>
    static const u8 LCD_ADDR = 0x3f;
    static const u8 LCD_INFO_LINELEN = 20;
    static const u8 LCD_INFO_NLINES = 4;
    static LiquidCrystal_PCF8574 lcd(LCD_ADDR);
#elif defined(FGTERM_USE_128x64)
    static const u8 PIN_LCD_CS = 9;
    static const u8 PIN_LCD_RESET = 8; // or U8G_PIN_NONE
    //U8G2_ST7920_128X64_1_HW_SPI lcd(U8G2_R0, /* CS=*/ 12, /* reset=*/ 10);
    U8GLIB_ST7920_128X64 lcd(PIN_LCD_CS, U8G_PIN_NONE, PIN_LCD_RESET); // use hardware SPI
    //U8GLIB_ST7920_128X64 lcd(13, 11, 12, U8G_PIN_NONE, 8); // software SPI
    static const u8 LCD_INFO_LINELEN = 32;
    static const u8 LCD_INFO_NLINES = 10;
    static const u8 LCD_LINE_HEIGHT = 6;
#else
#error wat
#endif

static char lcdbuf[LCD_INFO_NLINES][LCD_INFO_LINELEN+1];
static u8 curbufline = 0, curbufcol = 0;
static u8 wline = 0;



static char * _lcdnextline()
{
    ++curbufline;
    curbufline %= LCD_INFO_NLINES;
    if(wline < LCD_INFO_NLINES)
        ++wline;
    return lcdbuf[curbufline];
}

static void _lcdfinishprint(char *p, u8 written)
{
    if(const u8 remain = LCD_INFO_LINELEN - written)
        memset(p + written, ' ', remain);
}

static void _lcdprint(const char *p, u8 lcdline)
{
#ifdef FGTERM_USE_2004
    lcd.setCursor(0, lcdline);
    lcd.print(p);
#elif defined(FGTERM_USE_128x64)
    //lcd.setPrintPos(0, lcdline * LCD_LINE_HEIGHT);
    //lcd.print(p);

    //digitalWrite(PIN_LCD_CS, HIGH);
    lcd.drawStr( 0, (lcdline * LCD_LINE_HEIGHT) + (LCD_LINE_HEIGHT-1), p);
    //digitalWrite(PIN_LCD_CS, LOW);
#else
#error wat
#endif
}

static void _lcdprintline(u8 bufline, u8 lcdline)
{
    const char *p = lcdbuf[bufline];
    _lcdprint(p, lcdline);
}

static void _lcdflush2()
{
     for(u8 i = 0; i < LCD_INFO_NLINES; ++i)
        _lcdprintline((curbufline + i) % LCD_INFO_NLINES, (wline + i) % LCD_INFO_NLINES);
}

static void lcdflush()
{
#ifdef FGTERM_USE_128x64
    //digitalWrite(PIN_LCD_CS, HIGH);
    lcd.firstPage();
    do
        _lcdflush2();
    while(lcd.nextPage());
    //digitalWrite(PIN_LCD_CS, LOW);
#else
    _lcdflush2();
#endif
}

static void lcdclear()
{
    memset(lcdbuf, ' ', sizeof(lcdbuf));
    for(u8 i = 0; i < LCD_INFO_NLINES; ++i)
        lcdbuf[i][LCD_INFO_LINELEN] = 0;
    wline = 0;
    curbufline = 0;
    curbufcol = 0;
#ifdef FGTERM_USE_2004
    lcd.home();
    lcd.clear();
#elif defined(FGTERM_USE_128x64)
    lcd.setColorIndex(0);
    lcd.drawBox( 0, 0, lcd.getWidth(), lcd.getHeight() );
    lcd.setColorIndex(1);
    //lcd.setPrintPos(0, 0);
#else
#error wat
#endif
}


template<bool PGM>
struct ReadChar {};

template<> struct ReadChar<true> { inline static char get(const void *p) { return pgm_read_byte(p); } };
template<> struct ReadChar<false> { inline static char get(const void *p) { return *(char*)p; } };

inline static char readChar(const char *s, u8 offs) { return ReadChar<false>::get(s + offs); }
inline static char readChar(const __FlashStringHelper *s, u8 offs) { return ReadChar<true>::get(((const char*)s) + offs); }


template<typename TY>
struct LcdPrint
{
    static void print(const TY *s)
    {
        char *p = lcdbuf[curbufline];
        char c;
        u8 n = curbufcol;
        for(u8 i = 0; ((c = readChar(s, i))); ++i)
        {
            if(c == '\n')
                goto fin;

            p[n++] = c;

            if(n >= LCD_INFO_LINELEN)
            {
                fin:
                _lcdfinishprint(p, n);
                n = 0;
                p = _lcdnextline();
            }
        }
        curbufcol = n;
    }
};


static void print(const char *s, bool flush = true)
{
#ifdef FGTERM_USE_SERIAL
    Serial.print(s);
#endif
    LcdPrint<char>::print(s);
    if(flush)
        lcdflush();
}
static void print(const __FlashStringHelper *s, bool flush = true)
{
#ifdef FGTERM_USE_SERIAL
    Serial.print(s);
#endif
    LcdPrint<__FlashStringHelper>::print(s);
    if(flush)
        lcdflush();
}

static void newline(bool flush = true)
{
    print(F("\n"), flush);
}

static void printml(const char *buf, unsigned length)
{
    const bool full = length && length % LCD_INFO_LINELEN == 0;
    print(buf, full);
    if(!full)
        fgterm::newline();
#ifdef FGTERM_USE_SERIAL
    Serial.println();
#endif
}

static void printml(const char *buf)
{
    return printml(buf, strlen(buf));
}

void initLCD()
{
#ifdef FGTERM_USE_2004
    Wire.begin();
#ifdef FGTERM_USE_SERIAL
    Wire.beginTransmission(LCD_ADDR);
    u8 error = Wire.endTransmission();
    if (error)
        Serial.println(F("NO LCD FOUND"));
#endif
    lcd.begin(LCD_INFO_LINELEN, LCD_INFO_NLINES);
    lcd.setBacklight(50);

#elif defined(FGTERM_USE_128x64)
    lcd.begin();
    lcd.setColorIndex(1);
    lcd.setFont(u8g_font_4x6);
    //lcd.setRot180();
#endif

    /*for(u8 i = 0; ; i += 8)
    {
        char buf[64];
        sprintf(buf, "0123456789abcdef0123456789ab+%d", i);
        printml(buf);
    }*/

  lcdclear();
}


};
