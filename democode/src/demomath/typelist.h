#pragma once

#include "traits.h"

namespace fgstd {

template<typename T, T v>
struct ValToType { static constexpr T value = v; };

struct typelist_end;

template<typename HEAD, typename TAIL = typelist_end>
class typelist;

struct typelist_end
{
    enum { length = 0 };
    typedef void maxalign_type;
    typedef void Head;
    static size_t array_req_bytes(size_t) { return 0; }

    template<typename T>
    struct cons
    {
        typedef typelist<T, typelist_end> type;
    };
};

template<typename HEAD, typename TAIL, unsigned N>
struct _typelist_get
{
    typedef typename _typelist_get<typename TAIL::Head, typename TAIL::Tail, N-1>::type type;
};

template<typename HEAD, typename TAIL>
struct _typelist_get<HEAD, TAIL, 0u>
{
    typedef HEAD type;
};

template<typename T, typename HEAD, typename TAIL>
struct _typelist_count
{
    enum { value = !!is_same<HEAD, T>::value + _typelist_count<T, typename TAIL::Head, typename TAIL::Tail>::value };
};

template<typename T, typename HEAD>
struct _typelist_count<T, HEAD, typelist_end>
{
    enum { value = !!is_same<HEAD, T>::value };
};

template<typename TL, typename HEAD, typename TAIL>
struct _typelist_reverse
{
    typedef typename _typelist_reverse<typelist<HEAD, TL>, typename TAIL::Head, typename TAIL::Tail>::type type;
};

template<typename TL, typename HEAD>
struct _typelist_reverse<TL, HEAD, typelist_end>
{
    typedef typelist<HEAD, TL> type;
};

template<typename HEAD, typename TAIL, typename OTHER>
struct _typelist_concat
{
    typedef typename _typelist_concat<typename TAIL::Head, typename TAIL::Tail, OTHER>::type newtail;
    typedef typename typelist<HEAD, newtail>::type type;
};

template<typename HEAD, typename OTHER>
struct _typelist_concat<HEAD, typelist_end, OTHER>
{
    typedef typelist<HEAD, OTHER> type;
};

// first parameter is the thing to transform, the rest is optional and just passed through
template<typename T, typename STATE, template<typename...> class X, typename... Ps>
struct _single_transform
{
    typedef X<T, STATE, Ps...> transformed;
};
template<template<typename...> class X, typename STATE, typename HEAD, typename TAIL, typename... Ps>
struct _list_transform
{
    typedef typename _single_transform<HEAD, STATE, X, Ps...>::transformed transformed;
    typedef typename transformed::type newtype;
    typedef typename transformed::state newstate;
    typedef _list_transform<X, newstate, typename TAIL::Head, typename TAIL::Tail, Ps...> transtail;
    typedef typename transtail::type tailtype;
    typedef typename transtail::state tailstate;
    typedef typelist<newtype, tailtype> type;
    typedef tailstate state;
};
template<template<typename...> class X, typename STATE, typename HEAD, typename... Ps>
struct _list_transform<X, STATE, HEAD, typelist_end, Ps...>
{
    typedef typename _single_transform<HEAD, STATE, X, Ps...>::transformed transformed;
    typedef typename transformed::type newtype;
    typedef typename transformed::state newstate;
    typedef typelist<newtype, typelist_end> type;
    typedef typename transformed::state state;
};
template<template<typename...> class X, typename STATE, typename... Ps>
struct _list_transform<X, STATE, typelist_end, typelist_end, Ps...>
{
    typedef typelist_end type;
    typedef STATE state;
};


template<typename T, typename E>
struct elemvalue
{
    static constexpr T value = E::value;
};

template<typename T, typename E>
struct elemcastvalue
{
    static constexpr T value = static_cast<T>(E::value);
};

template<typename T, T... vals>
struct array
{
    enum { size = sizeof...(vals) };
    static constexpr T values[size] = { vals... };
};

template<typename T, template<typename, typename> class X, typename HEAD, typename TAIL, T... vals>
struct arraybuilder
{
    static constexpr T _value = X<T, HEAD>::value;
    typedef typename arraybuilder<T, X, typename TAIL::Head, typename TAIL::Tail, vals..., _value>::type type;
};
template<typename T, template<typename, typename> class X, typename HEAD, T... vals>
struct arraybuilder<T, X, HEAD, typelist_end, vals...>
{
    static constexpr T _value = X<T, HEAD>::value;
    typedef array<T, vals..., _value> type;
};
template<typename T, template<typename, typename> class X, T... vals>
struct arraybuilder<T, X, typelist_end, typelist_end, vals...>
{
    typedef array<T> type;
};

template<typename HEAD, typename TAIL>
class typelist
{
public:
    typedef HEAD Head;
    typedef TAIL Tail;

    template<unsigned N>
    struct get
    {
        typedef typename  _typelist_get<HEAD, TAIL, N>::type type;
    };

    template<typename T>
    struct count
    {
        enum { value = _typelist_count<T, HEAD, TAIL>::value };
    };

    template<typename T>
    struct cons
    {
        typedef typelist<T, typelist<Head, Tail> > type;
    };

    struct reverse
    {
        typedef typename _typelist_reverse<typelist_end, HEAD, TAIL>::type type;
    };

    template<typename TL>
    struct append
    {
        typedef typename _typelist_concat<HEAD, TAIL, TL>::type type;
    };

    template<template<typename...> class X, typename BEGINSTATE, typename... Ps>
    struct transform
    {
        typedef _list_transform<X, BEGINSTATE, Head, Tail, Ps...> _final;
        typedef typename _final::type type;
        typedef typename _final::state state;
    };

    template<typename T, template<typename, typename> class X = elemvalue>
    struct toarray : public arraybuilder<T, X, Head, Tail>::type {};



    enum
    {
        length = 1 + Tail::length,
    };
};

template<typename... Ts>
struct typelist_cons_v {};

template<>
struct typelist_cons_v<>
{
    typedef typelist_end type;
};

template<typename HEAD, typename... Ts>
struct typelist_cons_v<HEAD, Ts...>
{
    typedef typelist<HEAD,
        typename typelist_cons_v<Ts...>::type
    > type;
};

template<typename... Ts>
struct typelist_cons {};

template<typename... Ts>
struct typelist_cons<void (*) (Ts...)>
{
    typedef typename typelist_cons_v<Ts...>::type type;
};


#define FGSTD_TYPELIST(x) typename fgstd::typelist_cons<void(*) x>::type

}


