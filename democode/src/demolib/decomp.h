#pragma once

#include "fglcd/mcu.h"
#include "../demomath/traits.h"

#include "decomp_rle.h"
#include "decomp_tschunk.h"
#include "decomp_exo.h"
#include "decomp_mix.h"


template<typename Reader>
struct DecompState<PK_UNCOMP, Reader>
{
    const typename Reader::Pointer _src;
    const unsigned _size;
    FORCEINLINE DecompState(typename Reader::Pointer src, unsigned size)
        : _src(src), _size(size)
    {}

    template<DecompDst dd, unsigned WSZ>
    FORCEINLINE void decomp(void *dst)
    {
        typedef typename SelectWindowType<dd, PK_UNCOMP, WSZ>::type Window;
        Window w(dst);
        decomp(w);
    }

    template<typename Window>
    FORCEINLINE void decomp(Window& w)
    {
        w.emitLiterals(_src, _size);
    }
};

template<typename Reader>
struct DecompState<PK_RLE, Reader>
{
    const typename Reader::Pointer _src;
    FORCEINLINE DecompState(typename Reader::Pointer src, unsigned)
        : _src(src)
    {}

    template<DecompDst dd, unsigned WSZ>
    FORCEINLINE void decomp(void *dst)
    {
        typedef typename SelectWindowType<dd, PK_RLE, WSZ>::type Window;
        Window w(dst);
        decomp(w);
    }

    template<typename Window>
    FORCEINLINE void decomp(Window& w)
    {
        rledecomp<Window, Reader>(w, _src);
    }
};

template<typename Reader>
struct DecompState<PK_EXO, Reader>
{
private:
    typename Reader::Pointer _src;
    ExoState _exo;
public:
    FORCEINLINE DecompState(typename Reader::Pointer src, unsigned)
    {
        _src = exo_decrunch_init<Reader>(_exo, src);
    }

    template<DecompDst dd, unsigned WSZ>
    FORCEINLINE void decomp(void *dst)
    {
        typedef typename SelectWindowType<dd, PK_EXO, WSZ>::type Window;
        Window w(dst);
        decomp(w);
    }

    template<typename Window>
    FORCEINLINE void decomp(Window& w)
    {
        exo_decrunch<Reader, Window>(_exo, _src, w);
    }
};

template<typename Reader>
struct DecompState<PK_TSCHUNK, Reader>
{
    const typename Reader::Pointer _src;
    const unsigned _ncmd;
    FORCEINLINE DecompState(typename Reader::Pointer src, unsigned)
        : _src(src + 2), _ncmd(Reader::template read<uint16_t>(src))
    {
    }

    template<DecompDst dd, unsigned WSZ>
    FORCEINLINE void decomp(void *dst)
    {
        typedef typename SelectWindowType<dd, PK_TSCHUNK, WSZ>::type Window;
        Window w(dst);
        decomp(w);
    }

    template<typename Window>
    FORCEINLINE void decomp(Window& w)
    {
        tschunkdecomp<Reader, Window>(w, _src, _ncmd);
    }
};


template<PackType PK, unsigned WSZ, typename Reader>
FORCEINLINE void decompressRAM(void *dst, typename Reader::Pointer src, unsigned sz)
{
    DecompState<PK, Reader> dec(src, sz);
    dec.template decomp<ToRAM, WSZ>(dst);
}

template<PackType, typename TImage, unsigned extrasize = 0>
struct _DecompDataStorage
{
    uint8_t unpacked[TImage::fullsize + extrasize];
    typedef fglcd::RAM Mem;
    static const unsigned windowsize = unsigned(TImage::windowsize < TImage::fullsize ? TImage::windowsize : TImage::fullsize);

    void _prepare(unsigned decompToOffs = 0)
    {
        decompressRAM<TImage::packtype, windowsize, fglcd::ProgmemFar>(&unpacked[decompToOffs], fglcd_get_farptr(TImage::data), TImage::packedsize);
    }

    FORCEINLINE uint8_t *ptr() { return &unpacked[0]; }
    FORCEINLINE const uint8_t *ptr() const { return &unpacked[0]; }
};

// If the image is already uncompressed in ROM, no need to copy anything
template<typename TImage>
struct _DecompDataStorage<PK_UNCOMP, TImage>
{
    typedef fglcd::ProgmemFar Mem;
    FORCEINLINE static fglcd::FarPtr ptr() { return fglcd_get_farptr(TImage::data); }
    FORCEINLINE void _prepare() {}
};
