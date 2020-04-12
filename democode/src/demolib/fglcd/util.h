#pragma once

#include "def.h"
#include <stdint.h>

#ifdef FGLCD_DEBUG
void _assert_fail(const char *cond_P, const char *msg_P, unsigned line, int val, int val2); // must be defined by user somewhere
#endif

namespace fglcd {

template<uintmax_t N>
struct TypeForSize
{
    typedef typename TypeSwitch<
            N <= UINTMAX_C(0xff),
            u8,
            typename TypeSwitch<
                N <= UINTMAX_C(0xffff),
                u16,
                typename TypeSwitch<
                    N <= UINTMAX_C(0xffffffff),
                    u32,
                    u64
                >::type
            >::type
        >::type type;
};

template<uintmax_t A, uintmax_t B>
struct SizeConfig
{
    typedef typename TypeForSize<A * B>::type type;
    static const type total = type(A * B);
    static_assert(total == A*B, "value truncated");
    static_assert(total / A == B, "wrong eval");
};

template <typename T>
struct has_port_tag
{
    typedef char yes[1];
    typedef char no[2];

    template <typename C> static yes& test(typename C::is_port_tag*);
    template <typename> static no& test(...);

    enum { value = sizeof(test<T>(0)) == sizeof(yes) };
};

template <typename T>
struct has_command_tag
{
    typedef char yes[1];
    typedef char no[2];

    template <typename C> static yes& test(typename C::is_command_tag*);
    template <typename> static no& test(...);

    enum { value = sizeof(test<T>(0)) == sizeof(yes) };
};

template <typename T>
struct has_trigger_tag
{
    typedef char yes[1];
    typedef char no[2];

    template <typename C> static yes& test(typename C::is_trigger_tag*);
    template <typename> static no& test(...);

    enum { value = sizeof(test<T>(0)) == sizeof(yes) };
};

template <typename T>
struct has_pin_tag
{
    typedef char yes[1];
    typedef char no[2];

    template <typename C> static yes& test(typename C::is_pin_tag*);
    template <typename> static no& test(...);

    enum { value = sizeof(test<T>(0)) == sizeof(yes) };
};

template <typename T>
struct has_con_tag
{
    typedef char yes[1];
    typedef char no[2];

    template <typename C> static yes& test(typename C::is_con_tag*);
    template <typename> static no& test(...);

    enum { value = sizeof(test<T>(0)) == sizeof(yes) };
};

template <typename T>
struct has_connection_tag
{
    typedef char yes[1];
    typedef char no[2];

    template <typename C> static yes& test(typename C::is_connection_tag*);
    template <typename> static no& test(...);

    enum { value = sizeof(test<T>(0)) == sizeof(yes) };
};

// https://stackoverflow.com/questions/5100015/c-metafunction-to-determine-whether-a-type-is-callable
/*template<typename T>
struct is_callable
{
private:
    typedef char(&yes)[1];
    typedef char(&no)[2];

    struct Fallback { void operator()(); };
    struct Derived : T, Fallback { };

    template<typename U, U> struct Check;

    template<typename>
    static yes test(...);

    template<typename C>
    static no test(Check<void (Fallback::*)(), &C::operator()>*);

public:
    static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
};*/

struct RAIINop
{
    FORCEINLINE RAIINop() {}
    FORCEINLINE ~RAIINop() {}
};

} // end namespace fglcd
