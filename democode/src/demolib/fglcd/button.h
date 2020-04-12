#pragma once

#if 0 // FIXME: this is too mad and impractical to use

#include "mcu.h"


namespace fglcd {

template<typename Deriv, typename MOD, u8 debounce_>
class DButtonBase
{
public:
    static const u8 debounce = debounce_;
    mutable u8 _dc;
    FORCEINLINE bool pressed() const { return _dc >= debounce; }
    FORCEINLINE operator bool() const { return pressed(); }
protected:
    DButtonBase() : _dc(0) {}
    bool update() const
    {
        const bool down = MOD::get(static_cast<Deriv*>(this)->_getButton());
        u8 c = _dc;
        if(down && c < debounce)
            _dc = c + 1;
        else if(!down && c)
            _dc = c - 1;
        return pressed();
    }
};

template<typename Deriv, typename MOD>
class DButtonBase<Deriv, MOD, 0>
{
public:
    DButtonBase() {}
    bool pressed() const { return MOD::get(static_cast<const Deriv*>(this)->get()); }
    FORCEINLINE operator bool() const { return pressed(); }
    bool update() const { return pressed(); }
};

struct _ButtonNoMod
{
    static FORCEINLINE bool get(bool x) { return x; };
};

struct _ButtonInvertMod
{
    static FORCEINLINE bool get(bool x) { return !x; };
};

template<typename MOD, u8 debounce, typename F, F func>
class StaticFuncButtonBase : public DButtonBase<StaticFuncButtonBase<MOD, debounce, F, func>, MOD, debounce>
{
public:
    static FORCEINLINE bool _getButton() { return func(); }
};

template<typename MOD, u8 debounce, typename PIN>
class PinButtonBase : public DButtonBase<StaticFuncButtonBase<MOD, debounce, decltype(PIN::get), PIN::get>, MOD, debounce>
{
public:
    typedef PIN Pin;
    static_assert(has_pin_tag<PIN>::value, "not a pin");
};

/*
template<typename MOD, u8 debounce, typename T, typename M>
class MemberFuncButtonBase : public DButtonBase<MemberFuncButtonBase<MOD, debounce, T, M>, MOD, debounce>
{
public:
    MemberFuncButtonBase(T& obj, M *mth) : _obj(&obj), _mth(mth) {}
    FORCEINLINE bool _getButton() { return *_obj.*_mth(); }

private:
    T * const _obj;
    M * const _mth;
};
*/
template<typename MOD, u8 debounce, typename T>
class CallableFuncButtonBase : public DButtonBase<CallableFuncButtonBase<MOD, debounce, T>, MOD, debounce>
{
public:
    CallableFuncButtonBase(T& c) : _callable(c) {}
    FORCEINLINE bool _getButton() { return _callable(); }

private:
    T& _callable;
};


template<typename MOD, u8 debounce, typename T, typename... Ts>
struct ButtonTypeSelector
{
    typedef typename TypeSwitch<has_pin_tag<T>::value,
        PinButtonBase<MOD, debounce, T>,
        typename TypeSwitch<is_callable<T>::value,
            CallableFuncButtonBase<MOD, debounce, T>,
            void
        >::type
    >::type type;
};

template<u8 debounce, typename... Ts>
class LowButton : public ButtonTypeSelector<_ButtonInvertMod, debounce, Ts...>::type
{
public:
    typedef typename ButtonTypeSelector<_ButtonInvertMod, debounce, Ts...>::type Self;
    using Self::Self;
};

template<u8 debounce, typename... Ts>
class HighButton : public ButtonTypeSelector<_ButtonNoMod, debounce, Ts...>::type
{
public:
    typedef typename ButtonTypeSelector<_ButtonNoMod, debounce, Ts...>::type Self;
    using Self::Self;
};

}

#endif

