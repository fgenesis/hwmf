#pragma once

#include "fgmath.h"

/* A helper class holding information about a point of view and its direction */
class View
{
public:
    FORCEINLINE View() : origin(noinit), forwardNorm(noinit), upNorm(noinit), rightNorm(noinit) {}

    // This ensures we have a normalized up and right vector, and both are orthogonal to each
    // other and to forward.
    View(const vec3& origin, const vec3& at, const vec3 &u);
    void setForwardUp(const vec3& fwd, const vec3& u);

    FORCEINLINE void setOrigin(const vec3& o) { origin = o; }

    FORCEINLINE const vec3& getOrigin() const { return origin; }
    FORCEINLINE const vec3& getForwardNorm() const { return forwardNorm; }
    FORCEINLINE const vec3& getUpNorm() const { return upNorm; }
    FORCEINLINE const vec3& getRightNorm() const { return rightNorm; }

protected:
    vec3 origin;
    // known to be normalized vectors
    vec3 forwardNorm;
    vec3 upNorm;
    vec3 rightNorm;
};
