#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "fglcd/fglcd.h"
#include "drawhelper.h"
#include "../demomath/fgmath.h"

#undef abs

template<typename LCD>
struct DrawT
{
    typedef uint8_t u8;
    typedef int8_t s8;
    typedef uint16_t u16;
    typedef int16_t s16;
    typedef int32_t s32;

    typedef typename LCD::DimType DimType;
    typedef typename LCD::TotalPixelType TotalPixelType;
    typedef typename LCD::ColorType Color;
    static const DimType WIDTH = LCD::WIDTH;
    static const DimType HEIGHT = LCD::HEIGHT;
    static const DimType XMAX = LCD::XMAX;
    static const DimType YMAX = LCD::YMAX;

    static NOINLINE void palfade(Color * const pal, const u8 n, const u8 r, const u8 g, const u8 b)
    {
        u8 c[3];
        const u8 rgb[3] = { r, g, b };
        for(u8 i = 0; i < n; ++i)
        {
            LCD::splitcolor_inl(pal[i], &c[0]);
            for(u8 k = 0; k < 3; ++k)
                c[k] = saturateSub(c[k], rgb[k]);
            pal[i] = LCD::gencolor(c[0], c[1], c[2]);
        }
    }

    static FORCEINLINE void fillrectfromto(u16 x, u16 y, u16 x1, u16 y1, Color color)
    {
        LCD::fillrectfromto(x,y,x1,y1,color);
    }

    static NOINLINE void borderrectfromto(u16 x, u16 y, u16 x1, u16 y1, Color color, u8 thickness)
    {
        fillrectfromto(x, y, x1, y + thickness, color);
        fillrectfromto(x, y+thickness, x+thickness, y1-thickness, color);
        fillrectfromto(x1-thickness, y+thickness, x1, y1-thickness, color);
        fillrectfromto(x, y1-thickness, x1, y1, color);
    }

    // Based on http://andybrown.me.uk/2013/06/08/a-generic-16-bit-lcd-adaptor-for-the-arduino/
    // Uses two separate cases for horizontal and vertical lines to minimize the number of setxy() calls
    static void drawline(s16 x0, s16 y0, s16 x1, s16 y1, Color col)
    {
      if(x0 == x1)
      {
        LCD::fillrect(x0, vmin(y0, y1), 1, vabs(y1-y0)+1, col);
        return;
      }
  
      if(y0 == y1)
      {
        LCD::fillrect(vmin(x0, x1), y0, vabs(x0-x1)+1, 1, col);
        return;
      }
  
      const s16 dx = abs(x1 - x0);
      const s16 dy = abs(y1 - y0);
      if(dy < dx)
      {
        if(x0 > x1)
        {
          vswap(x0, x1);
          vswap(y0, y1);
        }
        //const s16 mdy = -dy;
        const s16 sy = y0 < y1 ? 1 : -1;
        s16 err = dx - dy;
        /*for( ; x0 < 0; )
        {
          const s16 err2 = err << 1;
          err -= dy;. 
          ++x0;
          if(err2 < dx)
          {
              err += dx;
              y0 += sy;
          }
        }*/
        const s16 xmax = vmin<s16>(x1, XMAX);
        //const s16 ymax = lcd::YMAX;
        //NoInterrupt no;
        for(;;)
        {
          LCD::setxy(x0, y0, x1, y0);
          LCD::setColor(col);
          fastcontx:
          LCD::sendPixel();
          if(x0 == xmax /*|| x0 >  int(XMAX)*/)
              break;
          const s16 err2 = err << 1;
          //if(err2 > mdy)
          {
              err -= dy;
              ++x0;
          }
          if(err2 < dx)
          {
              err += dx;
              y0 += sy;
              /*if(y0 > int(YMAX))
                  return;*/
          }
          else
            goto fastcontx;
        }
      }
      else
      {
        if(y0 > y1)
        {
          vswap(x0, x1);
          vswap(y0, y1);
        }
        //const s16 mdx = -dx;
        const s16 sx = x0 < x1 ? 1 : -1;
        s16 err = dy - dx;
    
        /*for( ; y0 < 0; )
        {
          const s16 err2 = err << 1;
          err -= dx;
          ++y0;
          if(err2 < dy)
          {
              err += dy;
              x0 += sx;
          }
        }*/

        //const s16 xmax = lcd::XMAX;
        const s16 ymax = vmin<s16>(y1, YMAX);
        //NoInterrupt no;
        for(;;)
        {
          LCD::setxy(x0, y0, x0, y1);
          LCD::setColor(col);
          fastconty:
          LCD::sendPixel();
          if(y0 == ymax /*|| y0 > int(YMAX)*/)
              break;
          const s16 err2 = err << 1;
          //if(err2 > mdx)
          {
              err -= dx;
              ++y0;
          }
          if(err2 < dy)
          {
              err += dy;
              x0 += sx;
              /*if(x0 > int(XMAX))
                  return;*/
          }
          else
            goto fastconty;
        }
      }
    }

    template<typename ImgReader>
    static FORCEINLINE void drawimageRaw(DimType x, DimType y, DimType w, DimType h, typename ImgReader::Pointer img)
    {
        // FIXME: use AABB clipping to screen
        LCD::setxywh_inl(x, y, w, h);
        _drawimageloopX16<ImgReader>(img, TotalPixelType(w) * TotalPixelType(h));
    }

    template<typename ImgReader>
    static FORCEINLINE void drawimageRaw(typename ImgReader::Pointer img, TotalPixelType n)
    {
        // FIXME: use AABB clipping to screen
        _drawimageloopX16<ImgReader>(img, n);
    }

    template<typename ImgReader>
    static NOINLINE void _drawimageloopX16(typename ImgReader::Pointer img, TotalPixelType n)
    {
        typename ImgReader::SegmentBackup seg;
        ImgReader::SetSegmentPtr(img);

        const u16 nx8 = u16(n / 8u);
        u8 rem = u8(n) % u8(8);
        for(u8 k = 0; k < 8; ++k)
        {
            FGLCD_DUFF8(DimType, nx8, {
                const u8 p = ImgReader::readIncFast(img);
                LCD::sendPixel(palgetcolor_inl(p));
            });
        }
        if(rem) do
            LCD::sendPixel(palgetcolor_inl(ImgReader::readIncFast(img)));
        while(--rem);
    }
}; // end Draw
