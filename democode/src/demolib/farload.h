#pragma once

#include "demo_def.h"

template<typename T, size_t N_>
struct _FarLoadHelper
{
    typedef int is_array_tag;
    enum { N = N_ };
    T data[N];
    FORCEINLINE       T& operator[](size_t i)       { return data[i]; }
    FORCEINLINE const T& operator[](size_t i) const { return data[i]; }
    FORCEINLINE       T* operator*()                { return &data[0]; }
    FORCEINLINE const T* operator*()          const { return &data[0]; }
    FORCEINLINE       T* ptr()                      { return &data[0]; }
    FORCEINLINE const T* ptr()                const { return &data[0]; }
};

template<typename T, size_t N>
FORCEINLINE _FarLoadHelper<T,N> _farload(fglcd::FarPtr p)
{
    _FarLoadHelper<T,N> ret;
    fglcd::ProgmemFar::Memcpy(&ret.data[0], p, N * sizeof(T));
    return ret;
}

#define farload(a) (_farload<typename fgstd::basic_type<decltype(a)>::type, Countof(a)>(fglcd_get_farptr(a)))
