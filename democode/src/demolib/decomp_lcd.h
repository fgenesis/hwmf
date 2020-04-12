#pragma once

#include "decomp.h"
#include "drawhelper.h"
#include "cfg-demo.h"

template<typename TheLCD> // emit pixels to LCD given a previously set palette
struct DecompToLCD
{   
    typedef typename TheLCD::ColorType Color;
    static FORCEINLINE void emitSingleByte(uint8_t c) { TheLCD::sendPixel(palgetcolor_inl(c)); }
    static FORCEINLINE void emitMultiByte(uint8_t c, uint16_t len) { TheLCD::setColor(palgetcolor_inl(c)); TheLCD::fastfill_u16(len); }
    //static FORCEINLINE void emitMultiByte(uint8_t c, uint8_t len) { TheLCD::setColor(palgetcolor_inl(c)); TheLCD::fastfill_u8(len); }
};

template<> struct SelectSideEffect<ToLCD> { typedef DecompToLCD<LCD> type; };


// -- Decomp entire image to RAM/LCD ---


// Unpack image to RAM
template<typename TImage>
struct DecompImageData : public _DecompDataStorage<TImage::packtype, TImage>, public TImage
{
    static_assert(!TImage::blocked, "image should be a single large block");
    typedef _DecompDataStorage<TImage::packtype, TImage> Storage;
    typedef typename Storage::Mem Mem;
    FORCEINLINE DecompImageData()
    {
        this->_prepare();
    }
    FORCEINLINE void applypal(uint8_t offs = 0)
    {
        applyPal16_Image<TImage>(offs);
    }
};

template<PackType PK, unsigned WSZ, typename Reader>
FORCEINLINE void decompressToLCD(void *dst, typename Reader::Pointer src, unsigned sz)
{
    DecompState<PK, Reader> dec(src, sz);
    dec.template decomp<ToLCD, WSZ>(dst);
}

template<typename TImage>
struct DrawImageHelper
{
    static const unsigned windowsize = unsigned(TImage::windowsize < TImage::fullsize ? TImage::windowsize : TImage::fullsize);
    static const unsigned w = TImage::w;
    static const unsigned h = TImage::h;

    FORCEINLINE void applypal()
    {
        applyPal16_Image<TImage>();
    }

    FORCEINLINE void draw(unsigned x, unsigned y)
    {
        LCD::setxywh(x, y, TImage::w, TImage::h);
        decompressToLCD<TImage::packtype, windowsize, fglcd::ProgmemFar>(NULL, fglcd_get_farptr(TImage::data), TImage::packedsize);
    }
};



// -- Decomp image blocks to RAM/LCD ---

template<typename TImage, bool blocked>
struct _DecompImageBlocksHelper
{
    static const unsigned totalblocks = 1;
};

template<typename TImage>
struct _DecompImageBlocksHelper<TImage, true>
{
    static const unsigned totalblocks = unsigned((uint32_t(TImage::w) * TImage::h) / (uint32_t(TImage::blockw) * TImage::blockh));
};

template<typename TImage>
struct DecompImageBlocks : public TImage
{
    static_assert(TImage::blocked, "image should consist of multiple blocks");

    static const unsigned _windowsize = unsigned(TImage::windowsize < TImage::fullsize ? TImage::windowsize : TImage::fullsize);
    typedef DecompState<TImage::packtype, fglcd::ProgmemFar> _Decomp;

    static const unsigned totalblocks = _DecompImageBlocksHelper<TImage, TImage::blocked>::totalblocks;

    const _Decomp dec;

    FORCEINLINE DecompImageBlocks()
        : dec(fglcd_get_farptr(TImage::data), TImage::packedsize)
    {
    }

    template<DecompDst dd>
    FORCEINLINE void unpackBlock(void *dst, unsigned block)
    {
        FGLCD_ASSERT_VAL2(block < totalblocks, "unpkblk", block, totalblocks);
        dec.template decompBlock<dd, _windowsize>(dst, block);
    }

    FORCEINLINE void toRAM(void *dst, unsigned block)
    {
        unpackBlock<ToRAM>(dst, block);
    }

    FORCEINLINE void toLCD(unsigned block)
    {
        unpackBlock<ToLCD>(NULL, block);
    }

    void applypal(uint8_t offs = 0)
    {
        applyPal16_Image<TImage>(offs);
    }
};

// bit of a hack to load blocks from 2 images with differing palettes
// layout: [ ImageA | ImageB ]
template<typename TImageA, typename TImageB>
struct DecompStitchedImageBlocks
{
    static const unsigned w = TImageA::w + TImageB::w;
    static const unsigned h = TImageA::h;
    //static_assert(h == TImageB::h);

    DecompImageBlocks<TImageA> _decA;
    DecompImageBlocks<TImageB> _decB;

    uint8_t _lastid;

    DecompStitchedImageBlocks()
        : _lastid(0xff)
    {
    }

    FORCEINLINE void applypal(uint8_t = 0) {}

    void _prep(uint8_t id)
    {
        if(id != _lastid)
        {
            _lastid = id;
            if(!id)
                _decA.applypal(0);
            else
                _decB.applypal(0);
        }
    }

    template<DecompDst dd>
    FORCEINLINE void unpackBlock(void *dst, unsigned block)
    {
        if(block < _decA.totalblocks)
        {
            _prep(0);
            _decA.template unpackBlock<dd>(dst, block);
        }
        else
        {
            _prep(1);
            _decB.template unpackBlock<dd>(dst, block - _decA.totalblocks);
        }
    }

    FORCEINLINE void toRAM(void *dst, unsigned block)
    {
        unpackBlock<ToRAM>(dst, block);
    }

    FORCEINLINE void toLCD(unsigned block)
    {
        unpackBlock<ToLCD>(NULL, block);
    }
};
