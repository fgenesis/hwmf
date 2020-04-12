#pragma once

#include "src/demolib/demo_def.h"

class CBM64Font
{
public:
    static const unsigned size_unpacked = 0x800;
    void loadCharset1();
    void loadCharset2();
    void drawchar(char c, const LCD::ColorType fg, const LCD::ColorType bg) const;
    LCD::PixelPos drawbuf(const uint8_t *s, unsigned len, LCD::DimType x, LCD::DimType y, const LCD::ColorType fg, const LCD::ColorType bg) const;

private:
    uint8_t _data[size_unpacked];
};
