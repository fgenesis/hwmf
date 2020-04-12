#define NO_SDL
#include "camera.h"
#include "fgmath.h"
#include <assert.h>

Camera::Camera(uint8_t fov, unsigned w, unsigned h)
: projection(tweakedInfinitePerspective_InvAspect(fov, fp1616(int(h)) / fp1616(int(w))))
, myup(0, -1, 0)
{
}

Camera::Camera(uint8_t fov)
: projection(tweakedInfinitePerspective_NoAspect(fov))
, myup(0, -1, 0)
{
}

void Camera::lookAt(const vec3& at)
{
    myforward = v.getOrigin().to(at);
    v.setForwardUp(myforward, myup);
}

void Camera::setUp(const vec3 & up)
{
    myup = up;
    v.setForwardUp(myforward, up);
}

Camera::Matrix Camera::calcMatrix() const
{
    //return projection * LookAt(v.getOrigin(), at, v.getUp());

    const vec3& me = v.getOrigin();
    const vec3& r = v.getRightNorm();
    const vec3& f = v.getForwardNorm();
    const vec3& u = v.getUpNorm();

    tmat4_lookat<fp1616> look;
#define S(x, y, v) look.template set<x, y>(v)
    S(0, 0, r.x);
    S(1, 0, r.y);
    S(2, 0, r.z);
    S(0, 1, u.x);
    S(1, 1, u.y);
    S(2, 1, u.z);
    S(0, 2, -f.x);
    S(1, 2, -f.y);
    S(2, 2, -f.z);
    S(3, 0, -dot(r, me));
    S(3, 1, -dot(u, me));
    S(3, 2, dot(f, me));
#undef S

    return projection * look;
}
