#pragma once

#ifdef __GNUC__
#define FGSTD_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define FGSTD_FORCE_INLINE __forceinline
#else
#define FGSTD_FORCE_INLINE inline
#endif

#define FGSTD_USE_CPP11

namespace fgstd {

template <typename T, T v>
struct IntegralConstant
{
    typedef T value_type;
    typedef IntegralConstant<T,v> type;
    static const T value = v;
};

typedef IntegralConstant<bool, true>  CompileTrue;
typedef IntegralConstant<bool, false> CompileFalse;
template<bool V> struct CompileCheck : IntegralConstant<bool, V>{};

template <bool B, typename T = void> struct enable_if { };
template <typename T>                struct enable_if<true, T> { typedef T type; };

template <typename T, typename U> struct is_same;
template <typename T, typename U> struct is_same      : CompileFalse { };
template <typename T>             struct is_same<T,T> : CompileTrue  { };

template <typename T> struct is_array           : CompileFalse {};
template <typename T> struct is_array<T[]>      : CompileTrue {};
template <typename T, size_t N> struct is_array<T[N]> : CompileTrue{};

template<typename T> struct is_reference        : CompileFalse {};
template<typename T> struct is_reference<T&>    : CompileTrue{};
#ifdef FGSTD_USE_CPP11
template<typename T> struct is_reference<T&&>   : CompileTrue{};
#endif

template<typename T> struct is_ptr        : CompileFalse {};
template<typename T> struct is_ptr<T*>    : CompileTrue{};

//template <typename T, T A, T B>   struct is_same_value : priv::IntegralConstant<bool, bool(A == B)>;

template<bool COND, typename A, typename B> struct TypeSwitch{};
template <typename A, typename B> struct TypeSwitch<true, A, B> { typedef A type; };
template <typename A, typename B> struct TypeSwitch<false, A, B> { typedef B type; };

template <typename T> struct remove_ref            { typedef T type; };
template <typename T> struct remove_ref<T&>        { typedef T type; };
#ifdef FGSTD_USE_CPP11
template <typename T> struct remove_ref<T&&>       { typedef T type; };
#endif

template <typename T> struct remove_const          { typedef T type; };
template <typename T> struct remove_const<const T> { typedef T type; };

template <typename T> struct remove_volatile             { typedef T type; };
template <typename T> struct remove_volatile<volatile T> { typedef T type; };

template <typename T> struct remove_ptr     { typedef T type; };
template <typename T> struct remove_ptr<T*> { typedef T type; };

template <typename T> struct remove_array { typedef T type; };
template <typename T> struct remove_array<T[]> { typedef typename remove_array<T>::type type; };
template <typename T, size_t N> struct remove_array<T[N]> { typedef typename remove_array<T>::type type; };


template <typename T> struct remove_cv
{
    typedef typename remove_const<
        typename remove_volatile<T>::type
    >::type type;
};

template<bool need_more, typename T> struct _get_basic_type_rec {};
template<typename T> struct _get_basic_type_rec<false, T> { typedef T type; };
template<typename T> struct _get_basic_type_rec<true, T> {
    typedef
        typename remove_array<
            typename remove_ref<
                typename remove_cv<
                    T
                >::type
            >::type
        >::type
    X;

    typedef typename _get_basic_type_rec<
        is_array<X>::value || is_reference<X>::value || is_ptr<X>::value,
        X
    >::type type;
};

template<typename T> struct basic_type : _get_basic_type_rec<true, T> {};



#ifdef FGSTD_USE_CPP11
template <class T>
typename remove_ref<T>::type&& move (T&& x)
{
    return static_cast<typename remove_ref<T>::type &&>(x);
}
#define FGSTD_MOVE(x) fgstd::move(x)
#else
#define FGSTD_MOVE(x) x
#endif


namespace detail
{

template<typename A>
struct has_swap_method
{
    typedef char yes[1];
    typedef char no[2];
    template<typename T, void (T::*)(T&)> struct SFINAE {};
    template<typename T> static yes& Test(SFINAE<T, &T::swap>*);
    template<typename T> static no&  Test(...);
    enum { value = sizeof(Test<A>(0)) == sizeof(yes) };
};

template<bool>
struct _swapper;

template<>
struct _swapper<true>
{
    template<typename T>
    static FGSTD_FORCE_INLINE void swap(T& a, T& b)
    {
        a.swap(b);
    }
};

template<>
struct _swapper<false>
{
    template<typename T>
    static FGSTD_FORCE_INLINE void swap(T& a, T& b)
    {
        T c(FGSTD_MOVE(a));
        a = FGSTD_MOVE(b);
        b = FGSTD_MOVE(c);
    }
};

} // end namespace detail


template<typename T>
FGSTD_FORCE_INLINE void swap(T& a, T& b)
{
    detail::_swapper<!!detail::has_swap_method<T>::value>::swap(a, b);
}

template<typename T = void>
struct swapper
{
    FGSTD_FORCE_INLINE void operator()(T& a, T& b) const
    {
        fgstd::swap(a, b);
    }
};
template<>
struct swapper<void>
{
    template<typename T>
    FGSTD_FORCE_INLINE void operator()(T& a, T& b) const
    {
        fgstd::swap(a, b);
    }
};

template<typename ITER>
void iter_swap(const ITER& a, const ITER& b)
{
    swap(*a, *b);
}


template<typename T = void>
struct less
{
    FGSTD_FORCE_INLINE bool operator()(const T& a, const T& b) const
    {
        return a < b;
    }
};
template<>
struct less<void>
{
    template<typename T>
    FGSTD_FORCE_INLINE bool operator()(const T& a, const T& b) const
    {
        return a < b;
    }
};

template<typename T = void>
struct equal
{
    FGSTD_FORCE_INLINE bool operator()(const T& a, const T& b) const
    {
        return a == b;
    }
};
template<>
struct equal<void>
{
    template<typename T>
    FGSTD_FORCE_INLINE bool operator()(const T& a, const T& b) const
    {
        return a == b;
    }
};

}
