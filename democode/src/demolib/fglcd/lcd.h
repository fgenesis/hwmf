#pragma once

#include "chip.h"

namespace fglcd {

template<typename CHIPX>
struct _LcdBase : public CHIPX
{
    typedef CHIPX Chip;
    typedef typename Chip::Specs Specs;
    typedef typename Chip::DataConnection DataConnection;
    typedef typename Chip::DimType DimType;
    typedef typename Chip::TotalPixelType TotalPixelType;
    typedef typename DataConnection::value_type ColorType;

    struct PixelPos
    {
        PixelPos() {}
        PixelPos(DimType a, DimType b) : x(a), y(b) {}
        DimType x, y;
    };

    // LCD commands modify PORTC and therefore leave garbage on the port,
    // therefore backup the color first, then change stuff, then put the color back
    struct StateBackup : private InsideInterrupt
    {
        const ColorType color;
        FORCEINLINE StateBackup()
            : color(getCurrentColor())
        {}

        ~StateBackup()
        {
            Chip::resumewrite();
            setColor(color);
        }
    };

    static const DimType WIDTH = Specs::LONG_SIZE; // TODO: use template for landscape/orientation selection themode
    static const DimType HEIGHT = Specs::SHORT_SIZE;
    static const DimType XMAX = Specs::LONG_SIZE - 1; // TODO: use template for landscape/orientation selection themode
    static const DimType YMAX = Specs::SHORT_SIZE - 1;
    static FORCEINLINE ColorType getCurrentColor() { return DataConnection::readOutput(); }
    static FORCEINLINE void setColor(ColorType c) { DataConnection::set(c); }
    static FORCEINLINE void sendPixel() { DataConnection::send(); }
    static FORCEINLINE void sendPixel(ColorType c) { DataConnection::send(c); }
    static FORCEINLINE void sendPixelAgain(ColorType c) { DataConnection::sendSameAgain(c); }

    static FORCEINLINE void preparefill()
    {
        // Forcing inlining here actually reduces code size
        Chip::setxy_inl(0, 0, XMAX, YMAX); 
    }

    static FORCEINLINE void fastfill_inl(TotalPixelType npix)
    {
        static constexpr TotalPixelType PIXELS_PER_ROUND = Chip::TOTAL_PIXELS / TotalPixelType(32); // unroll factor
        typedef typename TypeForSize<PIXELS_PER_ROUND>::type IterType;
        FGLCD_DUFF32_DIV(IterType, npix, sendPixel() );
    }

    static NOINLINE void fastfill_huge(TotalPixelType npix)
    {
        fastfill_inl(npix);
    }

    static NOINLINE void fastfill_u8(u8 npix)
    {
        FGLCD_DUFF16(u8, npix, sendPixel() );
    }

    static NOINLINE void fastfill_u16(u16 npix)
    {
        FGLCD_DUFF32(u16, npix, sendPixel() );
    }

    static NOINLINE void clear(ColorType color)
    {
        preparefill();
        setColor(color);
        fastfill_inl(Chip::TOTAL_PIXELS); // Forcing inline helps the compiler kill most of the duff device
    }

    static void fillrect(DimType x, DimType y, DimType w, DimType h, ColorType color)
    {
        Chip::setxywh(x, y, w, h);
        setColor(color);
        const TotalPixelType total = TotalPixelType(w) * h;
        fastfill_huge(total);
    }
    
    static void fillrectfromto(DimType x, DimType y, DimType x1, DimType y1, ColorType color)
    {
        Chip::setxy(x,y,x1, y1);
        setColor(color);
        const TotalPixelType total = TotalPixelType(x1-x+1)*TotalPixelType(y1-y+1);
        fastfill_huge(total);
    }
};

template<template<typename _Iface> class CHIP, typename Iface>
struct LcdBase : public _LcdBase<CHIP<Iface> >
{
};

} // end namespace fglcd
