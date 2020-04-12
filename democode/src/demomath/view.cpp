#define NO_SDL

#include "view.h"

View::View(const vec3 & origin, const vec3 & at, const vec3 & u)
{
    setOrigin(origin);
    setForwardUp(origin.to(at), u);
}

void View::setForwardUp(const vec3 & fwd, const vec3 & u)
{
    forwardNorm = normalize(fwd);
    upNorm = orthogonalize(forwardNorm, u, &rightNorm); // both vectors normalized
}
