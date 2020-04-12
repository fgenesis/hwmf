#pragma once

#include <stdlib.h>
#include "../demomath/traits.h"
#include "_algo_sort.h"

namespace fgstd {
 


template<typename ITER, typename LESS, typename SWAPPER>
void FGSTD_FORCE_INLINE sort(const ITER& begin, const ITER& end, const LESS& lt, const SWAPPER& swp)
{
    priv::quicksort(begin, end - 1, lt, swp);
}

template<typename ITER, typename LESS>
void FGSTD_FORCE_INLINE sort(const ITER& begin, const ITER& end, const LESS& lt)
{
    sort(begin, end, lt, swapper<>());
}

template<typename ITER>
void FGSTD_FORCE_INLINE sort(const ITER& begin, const ITER& end)
{
    sort(begin, end, less<>());
}




} // end namespace fgstd

