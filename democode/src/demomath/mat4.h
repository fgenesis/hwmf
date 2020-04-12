#pragma once

#include "typelist.h"
#include "vec.h"

template<typename T, typename DEF>
struct tmat4;

namespace detail {


enum ElemDef
{
    E_ZERO,
    E_ONE,
    E_DYNAMIC,
    E_MINUSONE,

    E0 = E_ZERO,
    E1 = E_ONE,
    ED = E_DYNAMIC,
    EM = E_MINUSONE
};

enum ElemSel
{
    S_CONST,
    S_FIRST,
    S_SECOND,
    S_BOTH,
    S_NEGATE_FIRST, // compiler can't optimize this because we're using custom types with inline asm
    S_NEGATE_SECOND // ^
};

template<ElemDef def> struct ElemDefType : fgstd::IntegralConstant<ElemDef, def> {};
typedef ElemDefType<E_DYNAMIC> ElemDynamicType;

template<ElemDef def, uint8_t dynidx>
struct ElemInfo : fgstd::IntegralConstant<ElemDef, def>
{
};

template<uint8_t dynidx>
struct ElemInfo<E_DYNAMIC, dynidx> : fgstd::IntegralConstant<ElemDef, E_DYNAMIC>
{
    enum : uint8_t { DynamicIndex = dynidx };
};


template<typename E, typename STATE>
struct _MakeElemInfo
{
    static const ElemDef defval = E::value;
    typedef ElemInfo<defval, STATE::value> type;
    typedef fgstd::IntegralConstant<uint8_t, STATE::value + (defval == E_DYNAMIC)> state;
};

template<ElemDef def, typename T>
struct ElemValueHelper {};

template<typename T>
struct ElemValueHelper<E_ZERO, T> { static FORCEINLINE constexpr T const_value() { return T(0); } };
template<typename T>
struct ElemValueHelper<E_ONE , T> { static FORCEINLINE constexpr T const_value() { return T(1); } };
template<typename T>
struct ElemValueHelper<E_MINUSONE , T> { static FORCEINLINE constexpr T const_value() { return T(-1); } };

template<typename OP, ElemDef def, ElemSel sel>
struct ElemOpHelper {};

template<typename OP, ElemDef def>
struct ElemOpHelper<OP, def, S_BOTH>
{
    template<typename A, typename B>
    static FORCEINLINE constexpr typename OP::template result_type<A, B>::type op(const A& a, const B& b)
    {
        return OP::template op(a, b);
    }
};

template<typename OP, ElemDef def>
struct ElemOpHelper<OP, def, S_FIRST>
{
    static_assert(def == E_DYNAMIC, "should be E_DYNAMIC");
    template<typename A, typename B>
    static FORCEINLINE constexpr const A& op(const A& a, const B&)
    {
        return a;
    }
};

template<typename OP, ElemDef def>
struct ElemOpHelper<OP, def, S_SECOND>
{
    static_assert(def == E_DYNAMIC, "should be E_DYNAMIC");
    template<typename A, typename B>
    static FORCEINLINE constexpr const B& op(const A&, const B& b)
    {
        return b;
    }
};

template<typename OP, ElemDef def>
struct ElemOpHelper<OP, def, S_NEGATE_FIRST>
{
    static_assert(def == E_DYNAMIC, "should be E_DYNAMIC");
    template<typename A, typename B>
    static FORCEINLINE constexpr const A op(const A& a, const B&)
    {
        return -a;
    }
};

template<typename OP, ElemDef def>
struct ElemOpHelper<OP, def, S_NEGATE_SECOND>
{
    static_assert(def == E_DYNAMIC, "should be E_DYNAMIC");
    template<typename A, typename B>
    static FORCEINLINE constexpr const B op(const A&, const B& b)
    {
        return -b;
    }
};

template<typename OP, ElemDef def>
struct ElemOpHelper<OP, def, S_CONST>
{
    template<typename A, typename B>
    static FORCEINLINE constexpr auto op(const A&, B&) -> decltype(ElemValueHelper<def, typename OP::template result_type<A, B>::type>::const_value())
    {
        return ElemValueHelper<def, typename OP::template result_type<A, B>::type>::const_value();
    }
};

template<ElemDef def, ElemSel sel>
struct ElemOpDef : public ElemDefType<def>
{
    static const ElemSel which = sel;
};

template<ElemDef def, ElemSel sel> struct ElemAddBase : ElemOpDef<def, sel>
{
    template<typename A, typename B>
    struct result_type
    {
        typedef decltype(A() + B()) type;
    };

    template<typename A, typename B>
    static FORCEINLINE constexpr auto op(const A& a, const B& b) -> decltype(a*b)
    {
        return a + b;
    }
};
template<ElemDef A, ElemDef B> struct ElemAdd {};
template<> struct ElemAdd<E_ZERO, E_ZERO> : ElemAddBase<E_ZERO, S_CONST> {};
template<> struct ElemAdd<E_ZERO, E_ONE> : ElemAddBase<E_ONE, S_CONST> {};
template<> struct ElemAdd<E_ZERO, E_DYNAMIC> : ElemAddBase<E_DYNAMIC, S_SECOND> {};
template<> struct ElemAdd<E_ZERO, E_MINUSONE> : ElemAddBase<E_MINUSONE, S_CONST> {};
template<> struct ElemAdd<E_ONE, E_ZERO> : ElemAddBase<E_ONE, S_CONST> {};
template<> struct ElemAdd<E_ONE, E_ONE> : ElemAddBase<E_DYNAMIC, S_BOTH> {}; // TODO: <E_CONST, S_CONST> ?
template<> struct ElemAdd<E_ONE, E_DYNAMIC> : ElemAddBase<E_DYNAMIC, S_BOTH> {};
template<> struct ElemAdd<E_ONE, E_MINUSONE> : ElemAddBase<E_ZERO, S_CONST> {};
template<> struct ElemAdd<E_DYNAMIC, E_ZERO> : ElemAddBase<E_DYNAMIC, S_FIRST> {};
template<> struct ElemAdd<E_DYNAMIC, E_ONE> : ElemAddBase<E_DYNAMIC, S_BOTH> {};
template<> struct ElemAdd<E_DYNAMIC, E_DYNAMIC> : ElemAddBase<E_DYNAMIC, S_BOTH> {};
template<> struct ElemAdd<E_DYNAMIC, E_MINUSONE> : ElemAddBase<E_DYNAMIC, S_BOTH> {};
template<> struct ElemAdd<E_MINUSONE, E_ZERO> : ElemAddBase<E_MINUSONE, S_CONST> {};
template<> struct ElemAdd<E_MINUSONE, E_ONE> : ElemAddBase<E_ZERO, S_CONST> {};
template<> struct ElemAdd<E_MINUSONE, E_DYNAMIC> : ElemAddBase<E_DYNAMIC, S_BOTH> {};
template<> struct ElemAdd<E_MINUSONE, E_MINUSONE> : ElemAddBase<E_DYNAMIC, S_BOTH> {};

template<ElemDef def, ElemSel sel> struct ElemMulBase : ElemOpDef<def, sel>
{
    template<typename A, typename B>
    struct result_type
    {
        typedef decltype(A() * B()) type;
    };

    template<typename A, typename B>
    static FORCEINLINE constexpr auto op(const A& a, const B& b) -> decltype(a*b)
    {
        return a * b;
    }
};
template<ElemDef A, ElemDef B> struct ElemMul {};
template<> struct ElemMul<E_ZERO, E_ZERO> : ElemMulBase<E_ZERO, S_CONST> {};
template<> struct ElemMul<E_ZERO, E_ONE> : ElemMulBase<E_ZERO, S_CONST> {};
template<> struct ElemMul<E_ZERO, E_DYNAMIC> : ElemMulBase<E_ZERO, S_CONST> {};
template<> struct ElemMul<E_ZERO, E_MINUSONE> : ElemMulBase<E_ZERO, S_CONST> {};
template<> struct ElemMul<E_ONE, E_ZERO> : ElemMulBase<E_ZERO, S_CONST> {};
template<> struct ElemMul<E_ONE, E_ONE> : ElemMulBase<E_ONE, S_CONST> {};
template<> struct ElemMul<E_ONE, E_DYNAMIC> : ElemMulBase<E_DYNAMIC, S_SECOND> {};
template<> struct ElemMul<E_ONE, E_MINUSONE> : ElemMulBase<E_MINUSONE, S_CONST> {};
template<> struct ElemMul<E_DYNAMIC, E_ZERO> : ElemMulBase<E_ZERO, S_CONST> {};
template<> struct ElemMul<E_DYNAMIC, E_ONE> : ElemMulBase<E_DYNAMIC, S_FIRST> {};
template<> struct ElemMul<E_DYNAMIC, E_DYNAMIC> : ElemMulBase<E_DYNAMIC, S_BOTH> {};
template<> struct ElemMul<E_DYNAMIC, E_MINUSONE> : ElemMulBase<E_DYNAMIC, S_NEGATE_FIRST> {};
template<> struct ElemMul<E_MINUSONE, E_ZERO> : ElemMulBase<E_ZERO, S_CONST> {};
template<> struct ElemMul<E_MINUSONE, E_ONE> : ElemMulBase<E_MINUSONE, S_CONST> {};
template<> struct ElemMul<E_MINUSONE, E_DYNAMIC> : ElemMulBase<E_DYNAMIC, S_NEGATE_SECOND> {};
template<> struct ElemMul<E_MINUSONE, E_MINUSONE> : ElemMulBase<E_ONE, S_CONST> {};

template<ElemDef def, typename EINFO>
struct getelem
{
    static_assert(EINFO::value == def, "plz");
    template<typename T>
    FORCEINLINE static constexpr T get(const T *) { return ElemValueHelper<def, T>::const_value(); }
};

template<typename EINFO>
struct getelem<E_DYNAMIC, EINFO>
{
    static_assert(EINFO::value == E_DYNAMIC, "must be E_DYNAMIC");
    template<typename T>
    FORCEINLINE static T get(const T *p)
    {
        return p[EINFO::DynamicIndex];
    }
};

template<ElemDef def, typename EINFO>
struct setelem {};

template<typename EINFO>
struct setelem<E_DYNAMIC, EINFO>
{
    static_assert(EINFO::value == E_DYNAMIC, "must be E_DYNAMIC");
    template<typename T>
    FORCEINLINE static void set(T *p, const T& v)
    {
        p[EINFO::DynamicIndex] = v;
    }
};

template<ElemDef A0, ElemDef A1, ElemDef A2, ElemDef A3,
         ElemDef A4, ElemDef A5, ElemDef A6, ElemDef A7,
         ElemDef A8, ElemDef A9, ElemDef A10, ElemDef A11,
         ElemDef A12, ElemDef A13, ElemDef A14, ElemDef A15>
struct mat4def
{
#define E(x) fgstd::IntegralConstant<ElemDef, x>
    typedef FGSTD_TYPELIST((E(A0), E(A1), E(A2), E(A3), E(A4), E(A5), E(A6), E(A7), E(A8), E(A9), E(A10), E(A11), E(A12), E(A13), E(A14), E(A15))) rawTL;
#undef E
    typedef typename rawTL::template transform<_MakeElemInfo, fgstd::IntegralConstant<uint8_t, 0> > _TLtrans;
    typedef typename _TLtrans::type TL;
    typedef typename _TLtrans::state TLstate;
    enum { NumDynamic = TLstate::value };

    template<uint8_t x, uint8_t y>
    struct gettype
    {
        enum { listidx = y * 4 + x };
        typedef typename TL::template get<listidx>::type type;
    };

    template<uint8_t x, uint8_t y>
    struct accessor
    {
        typedef typename gettype<x, y>::type etype;
        typedef getelem<etype::value, etype> getter;

        template<typename T>
        FORCEINLINE static constexpr T get(const T *p) { return getter::get(p); }
    };

    template<uint8_t x, uint8_t y>
    struct setvalue
    {
        typedef typename gettype<x, y>::type etype;
        typedef setelem<etype::value, etype> setter;

        template<typename T>
        FORCEINLINE static void set(T *p, const T& v) { setter::set(p, v); }
    };
};

template<typename MA, typename MB, uint8_t row, uint8_t col, uint8_t k>
struct mat4op_mul_row_col_rec
{
    static_assert(MA::is_matrix_check, "not matrix");
    static_assert(MB::is_matrix_check, "not matrix");

    typedef typename MA::template accessor<k, col> aa;
    typedef typename MB::template accessor<row, k> ab;
    typedef typename aa::etype aty;
    typedef typename ab::etype bty;
    typedef ElemMul<aty::value, bty::value> Mul;
    static const ElemDef value = Mul::value;
    static const ElemSel sel = Mul::which;
    typedef ElemOpHelper<Mul, value, sel> Op;
    typedef typename Mul::template result_type<typename MA::value_type, typename MB::value_type>::type result_type;

    static FORCEINLINE result_type getvalue(const MA& a, const MB& b)
    {
        const typename MA::value_type va = a.template get<k,col>();
        const typename MB::value_type vb = b.template get<row,k>();
        return Op::op(va, vb);
    }
};

template<typename MA, typename MB, uint8_t row, uint8_t col>
struct mat4op_mul_row_col
{
    static_assert(MA::is_matrix_check, "not matrix");
    static_assert(MB::is_matrix_check, "not matrix");

    typedef mat4op_mul_row_col_rec<MA, MB, row, col, 0> m0;
    typedef mat4op_mul_row_col_rec<MA, MB, row, col, 1> m1;
    typedef mat4op_mul_row_col_rec<MA, MB, row, col, 2> m2;
    typedef mat4op_mul_row_col_rec<MA, MB, row, col, 3> m3;
    static const ElemDef v0 = m0::value;
    static const ElemDef v1 = m1::value;
    static const ElemDef v2 = m2::value;
    static const ElemDef v3 = m3::value;
    typedef ElemAdd<v0, v1> a0;
    static const ElemDef tmp1 = a0::value;
    typedef ElemAdd<tmp1, v2> a1;
    static const ElemDef tmp2 = a1::value;
    typedef ElemAdd<tmp2, v3> a2;
    static const ElemDef value = a2::value;
    typedef typename a2::template result_type<typename m2::result_type, typename m3::result_type>::type result_type;

    static const ElemSel sel0 = a0::which;
    static const ElemSel sel1 = a1::which;
    static const ElemSel sel2 = a2::which;
    typedef ElemOpHelper<a0, tmp1, sel0> Op0;
    typedef ElemOpHelper<a1, tmp2, sel1> Op1;
    typedef ElemOpHelper<a2, value, sel2> Op2;

    static FORCEINLINE result_type getvalue(const MA& a, const MB& b)
    {
        //return m0::getvalue(a,b) + m1::getvalue(a,b) + m2::getvalue(a,b) + m3::getvalue(a,b);

        const typename m0::result_type mv0 = m0::getvalue(a,b);
        const typename m1::result_type mv1 = m1::getvalue(a,b);
        const typename m2::result_type mv2 = m2::getvalue(a,b);
        const typename m3::result_type mv3 = m3::getvalue(a,b);
        const result_type av0 = Op0::op(mv0, mv1);
        const result_type av1 = Op1::op(av0, mv2);
        const result_type av2 = Op2::op(av1, mv3);
        return av2;
    }
};

template<ElemDef def, uint8_t x, uint8_t y>
struct mat4_mul_filler2
{
    template<typename R, typename MA, typename MB>
    static FORCEINLINE void apply(R& m, const MA& a, const MB& b) {}
};

template<uint8_t x, uint8_t y>
struct mat4_mul_filler2<E_DYNAMIC,x,y>
{
    template<typename R, typename MA, typename MB>
    static FORCEINLINE void apply(R& m, const MA& a, const MB& b)
    {
        typedef mat4op_mul_row_col<MA, MB, x, y> getter;
        m.template set<x,y>(getter::getvalue(a, b));
    }
};

template<unsigned idx>
struct mat4_mul_filler
{
    static const uint8_t x = idx % 4;
    static const uint8_t y = idx / 4;
    static_assert(x < 4 && y < 4, "wat");
    template<typename R, typename MA, typename MB>
    static FORCEINLINE void apply(R& m, const MA& a, const MB& b)
    {
        typedef mat4op_mul_row_col<MA, MB, x, y> getter;
        mat4_mul_filler2<getter::value, x, y>::template apply(m, a, b);
        mat4_mul_filler<idx + 1>::template apply(m, a, b);
    }
};
template<>
struct mat4_mul_filler<16>
{
    template<typename R, typename MA, typename MB>
    static FORCEINLINE void apply(R& m, const MA& a, const MB& b) {}
};

template<typename MA, typename MB>
struct mat4op_mul
{
    static_assert(fgstd::is_same<typename MA::value_type, typename MB::value_type>::value, "matrix must have same value_type"); // TODO: relax this?
    template<uint8_t row, uint8_t col> struct elem : mat4op_mul_row_col<MA, MB, row, col> {};
#define E(x,y) (elem<x,y>::value)
#define EROW(y) E(0,y), E(1,y), E(2,y), E(3,y)
    typedef mat4def<EROW(0), EROW(1), EROW(2), EROW(3)> deftype;
#undef E
#undef EROW

    template<typename T> using resulttype = tmat4<T, deftype>;

    static resulttype<typename MA::value_type> getresult(const MA& a, const MB& b)
    {
        static_assert(MA::is_matrix_check, "not matrix");
        static_assert(MB::is_matrix_check, "not matrix");
        typedef resulttype<typename MA::value_type> mtype;
        static_assert(mtype::is_matrix_check, "result should be matrix");
        mtype m;
        mat4_mul_filler<0>::template apply<mtype, MA, MB>(m, a, b);
        /*if((m.template get<3, 3>()) <= mtype::Zero()) {
          //printf("invalid w value: %i\n", m.template get<3, 3>());
          assert(false);
        }*/
        return m;
    }
};

template<typename M, uint8_t x, uint8_t y>
struct mat4op_mul_vec4_rec
{
    static_assert(M::is_matrix_check, "not matrix");

    typedef typename M::mvec4 mvec4;
    typedef typename M::template accessor<x, y> aa;
    typedef typename aa::etype aty;
    typedef ElemMul<aty::value, E_DYNAMIC> Mul;
    static const ElemDef value = Mul::value;
    static const ElemSel sel = Mul::which;
    typedef ElemOpHelper<Mul, value, sel> Op;
    typedef typename Mul::template result_type<typename M::value_type, typename mvec4::value_type>::type result_type;

    template<typename S>
    static FORCEINLINE result_type getvalue(const M& a, const tvec4<S>& v)
    {
        const typename M::value_type va = a.template get<x,y>();
        return Op::op(va, v[x]);
    }
};

template<typename M, uint8_t y>
struct mat4op_mul_vec4_elem
{
    static_assert(M::is_matrix_check, "not matrix");

    typedef typename M::mvec4 mvec4;
    typedef mat4op_mul_vec4_rec<M, 0, y> m0;
    typedef mat4op_mul_vec4_rec<M, 1, y> m1;
    typedef mat4op_mul_vec4_rec<M, 2, y> m2;
    typedef mat4op_mul_vec4_rec<M, 3, y> m3;
    static const ElemDef v0 = m0::value;
    static const ElemDef v1 = m1::value;
    static const ElemDef v2 = m2::value;
    static const ElemDef v3 = m3::value;
    typedef ElemAdd<v0, v1> a0;
    static const ElemDef tmp1 = a0::value;
    typedef ElemAdd<tmp1, v2> a1;
    static const ElemDef tmp2 = a1::value;
    typedef ElemAdd<tmp2, v3> a2;
    static const ElemDef value = a2::value;
    typedef typename a2::template result_type<typename m2::result_type, typename m3::result_type>::type result_type;

    static const ElemSel sel0 = a0::which;
    static const ElemSel sel1 = a1::which;
    static const ElemSel sel2 = a2::which;
    typedef ElemOpHelper<a0, tmp1, sel0> Op0;
    typedef ElemOpHelper<a1, tmp2, sel1> Op1;
    typedef ElemOpHelper<a2, value, sel2> Op2;

    template<typename S>
    static FORCEINLINE result_type getvalue(const M& a, const tvec4<S>& v)
    {
        //return m0::getvalue(a,b) + m1::getvalue(a,b) + m2::getvalue(a,b) + m3::getvalue(a,b);

        const typename m0::result_type mv0 = m0::getvalue(a,v);
        const typename m1::result_type mv1 = m1::getvalue(a,v);
        const typename m2::result_type mv2 = m2::getvalue(a,v);
        const typename m3::result_type mv3 = m3::getvalue(a,v);
        const result_type av0 = Op0::op(mv0, mv1);
        const result_type av1 = Op1::op(av0, mv2);
        const result_type av2 = Op2::op(av1, mv3);
        return av2;
    }
};


template<typename M>
struct mat4op_mul_vec4
{
    typedef typename M::mvec4 mvec4;
    typedef mat4op_mul_vec4_elem<M, 0> e0;
    typedef mat4op_mul_vec4_elem<M, 1> e1;
    typedef mat4op_mul_vec4_elem<M, 2> e2;
    typedef mat4op_mul_vec4_elem<M, 3> e3;
    template<typename S>
    static mvec4 getresult(const M& m, const tvec4<S>& v)
    {
        mvec4 r(noinit);
        r[0] = e0::getvalue(m, v);
        r[1] = e1::getvalue(m, v);
        r[2] = e2::getvalue(m, v);
        r[3] = e3::getvalue(m, v);
        return r;
    }
};

typedef mat4def< E1, E0, E0, E0,
                 E0, E1, E0, E0,
                 E0, E0, E1, E0,
                 E0, E0, E0, E1 > def_identity;

typedef mat4def< ED, ED, ED, ED,
                 ED, ED, ED, ED,
                 ED, ED, ED, ED,
                 ED, ED, ED, ED > def_fulldynamic;

typedef mat4def< ED, E0, E0, E0,
                 E0, ED, E0, E0,
                 E0, E0, ED, E0,
                 E0, E0, E0, E1 > def_scale;

typedef mat4def< E1, E0, E0, E0,
                 E0, E1, E0, E0,
                 E0, E0, E1, E0,
                 E0, E0, E0, ED > def_downscale;

typedef mat4def< E1, E0, E0, ED,
                 E0, E1, E0, ED,
                 E0, E0, E1, ED,
                 E0, E0, E0, E1 > def_translate;

typedef mat4def< E1, E0, E0, E0,
                 E0, ED, ED, E0,
                 E0, ED, ED, E0,
                 E0, E0, E0, E1 > def_rotx;

typedef mat4def< ED, E0, ED, E0,
                 E0, E1, E0, E0,
                 ED, E0, ED, E0,
                 E0, E0, E0, E1 > def_roty;

typedef mat4def< ED, ED, E0, E0,
                 ED, ED, E0, E0,
                 E0, E0, E1, E0,
                 E0, E0, E0, E1 > def_rotz;

typedef mat4def< ED, ED, ED, E0,
                 ED, ED, ED, E0,
                 ED, ED, ED, E0,
                 E0, E0, E0, E1 > def_rotxyz;

typedef mat4def< ED, ED, ED, ED,
                 ED, ED, ED, ED,
                 ED, ED, ED, ED,
                 E0, E0, E0, EM > def_lookat; // HACK: last elem should be 1, but -1 fixes inverted-Z

typedef mat4def< ED, E0, E0, E0,
                 E0, ED, E0, E0,
                 E0, E0, EM, EM,
                 E0, E0, ED, E0 > def_persp;

typedef mat4def< ED, E0, E0, E0,
                 E0, ED, E0, E0,
                 E0, E0, E1, ED,
                 E0, E0, ED, E0 > def_infperspLH;


} // end namespace detail

template<typename T, typename DEF>
struct tmat4;

template<typename T>
struct tmat4_any : public tmat4<T, detail::def_fulldynamic>
{
};

template<typename T>
struct tmat4_ident : public tmat4<T, detail::def_identity>
{
};

template<typename T, typename DEF>
struct tmat4
{
    static const bool is_matrix_check = true;
    typedef tmat4<T, DEF> Self;
    typedef T value_type;
    typedef DEF Def;
    typedef typename Def::TL Infolist;
    typedef tvec4<value_type> mvec4;

    static FORCEINLINE T Zero() { return T(0); }
    static FORCEINLINE T One() { return T(1); }

    enum { dynamicsize = Def::NumDynamic };
    T _dyn[dynamicsize];

    template<uint8_t x, uint8_t y>
    struct accessor : Def::template accessor<x,y> {};

    template<uint8_t x, uint8_t y>
    FORCEINLINE T get() const
    {
        typedef accessor<x,y> acc;
        return acc::get(_dyn);
    }

    template<uint8_t x, uint8_t y>
    struct setter : Def::template setvalue<x,y> {};

    template<uint8_t x, uint8_t y>
    FORCEINLINE void set(const T& v)
    {
        typedef setter<x,y> sett;
        sett::set(_dyn, v);
    }

    template<typename OM>
    FORCEINLINE typename fgstd::enable_if<OM::is_matrix_check, typename detail::mat4op_mul<Self, OM>::template resulttype<Self::value_type> >::type operator*(const OM& o) const
    {
        return detail::mat4op_mul<Self, OM>::getresult(*this, o);
    }

    template<typename S>
    FORCEINLINE mvec4 operator*(const tvec4<S>& v) const
    {
        return detail::mat4op_mul_vec4<Self>::getresult(*this, v);
    }

    /* FIXME THIS IS SOMEHOW BROKEN (still uninited values in ret?!)
    tmat4_any<T> getfull() const
    {
        tmat4_any<T> ret;
#define $(x, y) ret.template set<x, y>(this->template get<x, y>());
#define $$(r) $(0, r); $(1, r); $(2, 0); $(3, 0);
        $$(0); $$(1); $$(2); $$(3)
#undef $$
#undef $
        return ret;
    }
    */

    template<typename Mem>
    FORCEINLINE void rawload(typename Mem::Pointer p)
    {
        Mem::Memcpy(&_dyn[0], p, sizeof(_dyn));
    }
};

template<typename T>
struct tmat4_trans : public tmat4<T, detail::def_translate>
{
    typedef tmat4<T, detail::def_translate> Base;

    FORCEINLINE tmat4_trans(const T& x, const T& y, const T& z)
    {
        this->template set<3, 0>(x);
        this->template set<3, 1>(y);
        this->template set<3, 2>(z);
    }
};

template<typename T>
struct tmat4_scale : public tmat4<T, detail::def_scale>
{
    FORCEINLINE tmat4_scale(const T& x)
    {
        this->template set<0, 0>(x);
        this->template set<1, 1>(x);
        this->template set<2, 2>(x);
    }

    FORCEINLINE tmat4_scale(const tvec3<T>& v)
    {
        this->template set<0, 0>(v.x);
        this->template set<1, 1>(v.y);
        this->template set<2, 2>(v.z);
    }
};

template<typename T>
struct tmat4_downscale : public tmat4<T, detail::def_downscale>
{
  FORCEINLINE tmat4_downscale(const T& x)
  {
    this->template set<3, 3>(x);
  }
};

template<typename T>
struct tmat4_rotx : public tmat4<T, detail::def_rotx>
{
    template<typename SINCOS>
    tmat4_rotx(const typename SINCOS::angle_type& a, const SINCOS& lut)
    {
        const T c = lut.cos(a);
        const T s = lut.sin(a);
        this->template set<1, 1>(c);
        this->template set<2, 2>(c);
        this->template set<1, 2>(s);
        this->template set<2, 1>(-s);
    };
};

template<typename T>
struct tmat4_roty : public tmat4<T, detail::def_roty>
{
    template<typename SINCOS>
    tmat4_roty(const typename SINCOS::angle_type& a, const SINCOS& lut)
    {
        const T c = lut.cos(a);
        const T s = lut.sin(a);
        this->template set<0, 0>(c);
        this->template set<2, 2>(c);
        this->template set<2, 0>(s);
        this->template set<0, 2>(-s);
    };
};

template<typename T>
struct tmat4_rotz : public tmat4<T, detail::def_rotz>
{
    template<typename SINCOS>
    tmat4_rotz(const typename SINCOS::angle_type& a, const SINCOS& lut)
    {
        const T c = lut.cos(a);
        const T s = lut.sin(a);
        this->template set<0, 0>(c);
        this->template set<1, 1>(c);
        this->template set<0, 1>(s);
        this->template set<1, 0>(-s);
    };
};

template<typename T>
struct tmat4_lookat : public tmat4<T, detail::def_lookat>
{
};

template<typename T>
struct tmat4_infperspLH : public tmat4<T, detail::def_infperspLH>
{
};

template<typename T>
struct tmat4_persp : public tmat4<T, detail::def_persp>
{
};
