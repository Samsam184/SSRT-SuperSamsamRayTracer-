#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "ray.h"
#include "vec3.h"

class aabb {
public:
    interval x, y, z;

    aabb() {}

    aabb(const interval& x, const interval& y, const interval& z)
        : x(x), y(y), z(z) {
        pad_to_minimums();
    }

    aabb(const vec3& a, const vec3& b) {
        x = (a[0] <= b[0]) ? interval(a[0], b[0]) : interval(b[0], a[0]);
        y = (a[1] <= b[1]) ? interval(a[1], b[1]) : interval(b[1], a[1]);
        z = (a[2] <= b[2]) ? interval(a[2], b[2]) : interval(b[2], a[2]);
        pad_to_minimums();
    }

    // ------------------------------------------------------------
    //  AJOUTS indispensables pour mesh / triangle / transform
    // ------------------------------------------------------------

    // Renvoie le point minimum du AABB
    vec3 min() const {
        return vec3(x.min, y.min, z.min);
    }

    // Renvoie le point maximum du AABB
    vec3 max() const {
        return vec3(x.max, y.max, z.max);
    }

    // Fusion de deux AABB dans un constructeur (comme RTOW3)
    aabb(const aabb& box0, const aabb& box1) {
        x = interval(fmin(box0.x.min, box1.x.min),
            fmax(box0.x.max, box1.x.max));
        y = interval(fmin(box0.y.min, box1.y.min),
            fmax(box0.y.max, box1.y.max));
        z = interval(fmin(box0.z.min, box1.z.min),
            fmax(box0.z.max, box1.z.max));
    }

    // ------------------------------------------------------------

    const interval& axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    bool hit(const ray& r, interval ray_t) const {
        const vec3& ray_orig = r.origin();
        const vec3& ray_dir = r.direction();

        for (int axis = 0; axis < 3; axis++) {
            const interval& ax = axis_interval(axis);
            const double adinv = 1.0 / ray_dir[axis];

            auto t0 = (ax.min - ray_orig[axis]) * adinv;
            auto t1 = (ax.max - ray_orig[axis]) * adinv;

            if (t0 < t1) {
                if (t0 > ray_t.min) ray_t.min = t0;
                if (t1 < ray_t.max) ray_t.max = t1;
            }
            else {
                if (t1 > ray_t.min) ray_t.min = t1;
                if (t0 < ray_t.max) ray_t.max = t0;
            }

            if (ray_t.max <= ray_t.min)
                return false;
        }
        return true;
    }

    static aabb surrounding_box(const aabb& box0, const aabb& box1) {
        vec3 small(fmin(box0.x.min, box1.x.min),
            fmin(box0.y.min, box1.y.min),
            fmin(box0.z.min, box1.z.min));

        vec3 big(fmax(box0.x.max, box1.x.max),
            fmax(box0.y.max, box1.y.max),
            fmax(box0.z.max, box1.z.max));

        return aabb(small, big);
    }

    int longest_axis() const {
        if (x.size() > y.size())
            return x.size() > z.size() ? 0 : 2;
        else
            return y.size() > z.size() ? 1 : 2;
    }

    static const aabb empty, universe;

private:
    void pad_to_minimums() {
        double delta = 0.0001;
        if (x.size() < delta) x = x.expand(delta);
        if (y.size() < delta) y = y.expand(delta);
        if (z.size() < delta) z = z.expand(delta);
    }
};

const aabb aabb::empty = aabb(interval::empty, interval::empty, interval::empty);
const aabb aabb::universe = aabb(interval::universe, interval::universe, interval::universe);

#endif
