#pragma once

#include "fgmath.h"
#include "mat4.h"
#include "view.h"

class Camera
{
    typedef tmat4_persp<fp1616> ProjMatrix;

public:

    Camera(uint8_t fov, unsigned w, unsigned h);
    Camera(uint8_t fov);
    FORCEINLINE void pos(const vec3& pos) { v.setOrigin(pos); }
    void lookAt(const vec3& at);

    FORCEINLINE const vec3& getPos() const { return v.getOrigin(); }

    FORCEINLINE const vec3& getForwardNorm() const { return v.getForwardNorm(); }
    FORCEINLINE const vec3& getUpNorm() const { return v.getUpNorm(); }
    FORCEINLINE const vec3& getRightNorm() const { return v.getRightNorm(); }

    void setUp(const vec3& up);

    typedef decltype(ProjMatrix() * tmat4_lookat<fp1616>()) Matrix;
    Matrix calcMatrix() const;

public:
    View  v;
    const ProjMatrix projection;

    vec3 myup, myforward; // not normalized
};

