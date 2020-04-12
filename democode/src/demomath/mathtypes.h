#include "vec.h"
#include "fp16_16.h"
#include "fp8_8.h"
#include "mat4.h"

// Runtime representation
typedef tvec2<fp1616> vec2;
typedef tvec3<fp1616> vec3;
typedef tvec4<fp1616> vec4;

// “Short vec”, i.e. storage format
typedef tvec2<fp88> svec2;
typedef tvec3<fp88> svec3;
typedef tvec4<fp88> svec4;

// float vectors, used by the demotools for generating integer models
typedef tvec2<float> fvec2;
typedef tvec3<float> fvec3;
typedef tvec4<float> fvec4;

// Integer vectors, used for screen coordinates
typedef tvec2<int16_t> ivec2;
typedef tvec3<int16_t> ivec3;
typedef tvec4<int16_t> ivec4;

typedef tvec2<int32_t> i32vec2;
typedef tvec3<int32_t> i32vec3;
typedef tvec4<int32_t> i32vec4;

typedef tvec2<uint16_t> uvec2;
typedef tvec3<uint16_t> uvec3;
typedef tvec4<uint16_t> uvec4;

typedef tvec2<uint8_t> u8vec2;
typedef tvec3<uint8_t> u8vec3;
typedef tvec4<uint8_t> u8vec4;

typedef tmat4_any<fp1616> mat4;
typedef tmat4_downscale<fp1616> mat4ds;
typedef tmat4_ident<fp1616> mat4i;
typedef tmat4_scale<fp1616> mat4s;
typedef tmat4_trans<fp1616> mat4t;
typedef tmat4_rotx<fp1616> mat4rx;
typedef tmat4_roty<fp1616> mat4ry;
typedef tmat4_rotz<fp1616> mat4rz;
