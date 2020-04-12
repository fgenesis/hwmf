#pragma once

#include <math.h>
#include <stdint.h>
#include "../demolib/demo_def.h"
#include "mathfuncs.h"

typedef uint8_t vecitype;

#define TFORALLS(a, b, op) do { for(vecitype i = 0; i < elems; ++i) { (a).ptr()[i] op b; }} while(0)
#define TCLONERETS(b, op) do { V a = asCVR(); TFORALL(a, b, +=); return a; } while(0)
#define TFORALL(a, b, op) TFORALLS(a, (b).cptr()[i], op)
#define TCLONERET(b, op) TCLONERETS((b).cptr()[i], op)

enum NoInit { noinit };

struct tvecbase {};

template<typename T, unsigned N>
struct tvecn : public tvecbase
{
    T e[N];
};

template<typename T, unsigned N, typename V>
struct vecops
{
    typedef T value_type;
    enum { elems = N };

    inline V& operator=(const V& b) { TFORALL(*this, b, =); return asVR(); }

    inline V operator-() const { V a(noinit); for(vecitype i = 0; i < elems; ++i) { a[i] = -(*this)[i]; } return a; }
    inline V operator+() const { V a(noinit); for(vecitype i = 0; i < elems; ++i) { a[i] = +(*this)[i]; } return a; }

    inline V operator+(const V& b) const { V a = asCVR(); TFORALL(a, b, +=); return a; }
    inline V operator-(const V& b) const { V a = asCVR(); TFORALL(a, b, -=); return a; }
    inline V operator*(const V& b) const { V a = asCVR(); TFORALL(a, b, *=); return a; }
    inline V operator/(const V& b) const { V a = asCVR(); TFORALL(a, b, /=); return a; }

    inline V operator+(const T& b) const { V a = asCVR(); TFORALLS(a, b, +=); return a; }
    inline V operator-(const T& b) const { V a = asCVR(); TFORALLS(a, b, -=); return a; }
    inline V operator*(const T& b) const { V a = asCVR(); TFORALLS(a, b, *=); return a; }
    inline V operator/(const T& b) const { V a = asCVR(); TFORALLS(a, b, /=); return a; }

    inline V& operator+=(const V& b) { TFORALL(*this, b, +=); return asVR(); }
    inline V& operator-=(const V& b) { TFORALL(*this, b, -=); return asVR(); }
    inline V& operator*=(const V& b) { TFORALL(*this, b, *=); return asVR(); }
    inline V& operator/=(const V& b) { TFORALL(*this, b, /=); return asVR(); }

    inline V& operator+=(const T& b) { TFORALLS(*this, b, +=); return asVR(); }
    inline V& operator-=(const T& b) { TFORALLS(*this, b, -=); return asVR(); }
    inline V& operator*=(const T& b) { TFORALLS(*this, b, *=); return asVR(); }
    inline V& operator/=(const T& b) { TFORALLS(*this, b, /=); return asVR(); }


    FORCEINLINE const T& operator[](vecitype i) const { return cptr()[i]; }
    FORCEINLINE       T& operator[](vecitype i)       { return ptr()[i]; }

    FORCEINLINE V to(const V& other) const { return other.asCVR() - this->asCVR(); }

    inline bool operator==(const V& o) const
    {
        for(vecitype i = 0; i < N; ++i)
            if((*this)[i] != o[i])
                return false;
        return true;
    }
    FORCEINLINE bool operator!=(const V& o) { return !(*this == o); }

private:
    FORCEINLINE V *asV() { return static_cast<V*>(this); }
    FORCEINLINE const V *asCV() const { return static_cast<const V*>(this); }
    FORCEINLINE V& asVR() { return *asV(); }
    FORCEINLINE const V& asCVR() const { return *asCV(); }

    FORCEINLINE T *ptr() { return reinterpret_cast<T*>(asV()); }
    FORCEINLINE const T *cptr() const { return reinterpret_cast<const T*>(asCV()); }
};

template<typename V>
inline typename V::value_type dot(const V& a, const V& b)
{
    typename V::value_type accu(0);
    for(vecitype i = 0; i < V::elems; ++i)
        accu += (a[i] * b[i]);
    return accu;
}

template<typename V>
inline typename V::value_type selfdot(const V& a)
{
    typename V::value_type accu(0);
    for(vecitype i = 0; i < V::elems; ++i)
    {
        const typename V::value_type v = a[i];
        accu += v * v;
    }
    return accu;
}

template<typename V>
FORCEINLINE typename V::value_type length(const V& v)
{
    return sqrt(selfdot(v));
}
template<typename V>
FORCEINLINE typename V::value_type distance(const V& a, const V& b)
{
    return length(a.to(b));
}

template<typename V>
inline V normalize(const V& v)
{
#if 0
    typename V::value_type len = length(v);
    if(!len)
        return V(0);
    return v / len;
#else
    typename V::value_type invlen = invsqrt(selfdot(v));
    if(!invlen)
        return V(0);
    return v * invlen;
#endif
}

template<typename T>
struct tvec2 : public vecops<T, 2, tvec2<T> >
{
    FORCEINLINE tvec2(NoInit) {}
    tvec2() { x = T(0); y = T(0); }
    explicit tvec2(T p) {  x=p; y=p; }
    tvec2(T p1, T p2) { x = p1; y = p2; }
    tvec2(const tvec2& o) { v = o.v; }

    template<typename X>
    explicit tvec2(const tvec2<X>& o) { x = T(o.x); y = T(o.y); }

    union
    {
        struct { T x,y; };
        struct { T r,g; };
        tvecn<T, 2> v;
    };

    FORCEINLINE tvec2<T>& operator=(const tvec2<T>& o) { x = o.x; y = o.y; return *this; }
};

template<typename T>
struct tvec3 : public vecops<T, 3, tvec3<T> >
{
    FORCEINLINE tvec3(NoInit) {}
    tvec3() { x = T(0); y = T(0); z = T(0);  }
    explicit tvec3(T p) { x=p; y=p; z=p; }
    tvec3(T p1, T p2, T p3) { x = p1; y = p2; z = p3; }
    tvec3(const tvec3& o) { v = o.v; }

    template<typename X>
    explicit tvec3(const tvec3<X>& o) { x = T(o.x); y = T(o.y); z = T(o.z); }

    union
    {
        struct { T x,y,z; };
        struct { T r,g,b; };
        struct { T s,t,p; };
        tvecn<T, 3> v;
    };

    FORCEINLINE tvec3<T>& operator=(const tvec3<T>& o) { x = o.x; y = o.y; z = o.z; return *this; }
};

template<typename T>
struct tvec4 : public vecops<T, 4, tvec4<T> >
{
    FORCEINLINE tvec4(NoInit) {}
    tvec4() { x = T(0); y = T(0); z = T(0); w = T(0);  }
    explicit tvec4(T p) { x=p; y=p; z=p; w=p; }
    tvec4(T p1, T p2, T p3, T p4) { x = p1; y = p2; z = p3; w = p4; }
    tvec4(const tvec4& o) { v = o.v; }
    //FORCEINLINE tvec4(const tvec3<T>& o) { x = o.x; y = o.y; z = o.z; w = T(1); }

    template<typename X>
    explicit tvec4(const tvec4<X>& o) { x = T(o.x); y = T(o.y); z = T(o.z); w = T(o.w);}

    union
    {
        struct { T x,y,z,w; };
        struct { T r,g,b,a; };
        struct { T s,t,p,q; };
        tvecn<T, 4> v;
    };

    FORCEINLINE tvec4<T>& operator=(const tvec4<T>& o) { x = o.x; y = o.y; z = o.z; w = o.w; return *this; }
};

#undef TFORALL
#undef TCLONERET


template<typename T>
static tvec3<T> cross(const tvec3<T>& x, const tvec3<T>& y)
{
    return tvec3<T>(
        x.y * y.z - y.y * x.z,
        x.z * y.x - y.z * x.x,
        x.x * y.y - y.x * x.y
    );
}

// ref must be normalized, then the returned vector and third will also be normalized
template<typename T>
static tvec3<T> orthogonalize(const tvec3<T> &ref, const tvec3<T> &toOrtho, tvec3<T> *third)
{
    const tvec3<T> v = normalize(cross(ref, toOrtho));
    *third = v;
    return cross(v, ref);
}
