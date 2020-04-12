#pragma once

#include <assert.h>

namespace fgstd {
namespace priv {

template<typename ITER>
ITER pickcenterelem(const ITER& left, const ITER& right)
{
    return left + ((right - left + 1) / 2u);
}

template<typename ITER, typename LESS, typename SWAPPER>
void medianswap3(const ITER& a, const ITER& b, const ITER& c, const LESS& lt, const SWAPPER& swp)
{
    if(lt(*b, *a))
        swp(*b, *a);
    if(lt(*c, *b))
        swp(*c, *b);
    if(lt(*b, *a))
        swp(*b, *a);
}

template<typename ITER, typename LESS, typename SWAPPER>
ITER partition(ITER left, ITER right, const LESS& lt, const SWAPPER& swp)
{
    ITER pivot = pickcenterelem(left, right);
    medianswap3(left, pivot, right, lt, swp);

    const auto pivotval = *pivot;
    while(true)
    {
        do
            ++left;
        while(lt(*left, pivotval));
        do
            --right;
        while(lt(pivotval, *right));
        if(left < right)
            swp(*left, *right);
        else
            return right;
    }
}


template<typename ITER, typename LESS, typename SWAPPER>
void quicksort(ITER left, ITER right, const LESS& lt, const SWAPPER& swp)
{
    while(left < right)
    {
        switch(right - left)
        {
            case 1:
                if(lt(*right, *left))
                    swp(*left, *right);
                return;
            case 2:
                medianswap3(left, left + 1, right, lt, swp);
                return;
        }
        ITER mid = partition(left, right, lt, swp);

        if(mid - left < right - mid)
        {
            quicksort(left, mid, lt, swp); // recurse into smaller left half
            left = mid + 1;           // "tail recurse" into bigger right half
        }
        else
        {
            quicksort(mid + 1, right, lt, swp);   // recurse into smaller right half
            right = mid;             // "tail recurse" into bigger left half
        }
    }
}



} // end namespace priv



} // end namespace fgstd
