#ifndef RAY_H
#define RAY_H

#include "vec3.h"


class ray {
public:
    inline ray() noexcept {}

    inline ray(const point3& origin, const vec3& direction, double time) noexcept : orig(origin), dir(direction), tm(time) {}
    inline ray(const point3& origin, const vec3& direction) noexcept : ray(origin, direction, 0) {}


    inline const point3& origin() const noexcept { return orig; }
    inline const vec3& direction() const noexcept { return dir; }

    inline double time() const noexcept { return tm; }

    inline __forceinline point3 at(double t) const noexcept {
        return orig + t * dir;
    }

private:
    point3 orig;
    vec3 dir;
    double tm;
};


#endif