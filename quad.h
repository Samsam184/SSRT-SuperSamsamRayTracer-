#ifndef QUAD_H
#define QUAD_H

#include "hittable.h"
#include "hittable_list.h"

class quad : public hittable {
public:
    quad(const vec3& Q, const vec3& u, const vec3& v, shared_ptr<material> mat)
        : Q(Q), u(u), v(v), mat(mat)
    {
        auto n = cross(u, v);
        normal = unit_vector(n);
        D = dot(normal, Q);
        w = n / dot(n, n);
        set_bounding_box();
        
        uint64_t local_state = rng_state.load();
        uint64_t rnd = xorshift64(local_state);
        rng_state.store(local_state);
        object_id = static_cast<int>(rnd % 256);

        float r = ((rnd >> 16) & 0xFF) / 255.0f;
        float g = ((rnd >> 8) & 0xFF) / 255.0f;
        float b = ((rnd & 0xFF)) / 255.0f;
        object_color = color(r, g, b);
        
    }

    void set_bounding_box() {
        auto bbox_diagonal1 = aabb(Q, Q + u + v);
        auto bbox_diagonal2 = aabb(Q + u, Q + v);
        bbox = aabb::surrounding_box(bbox_diagonal1, bbox_diagonal2);
    }

    aabb bounding_box() const override { return bbox; }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
        auto denom = dot(normal, r.direction());
        if (fabs(denom) < 1e-8) return false;

        auto t = (D - dot(normal, r.origin())) / denom;
        if (!ray_t.contains(t)) return false;

        auto intersection = r.at(t);
        vec3 planar_hitpt_vector = intersection - Q;
        auto alpha = dot(w, cross(planar_hitpt_vector, v));
        auto beta = dot(w, cross(u, planar_hitpt_vector));

        if (!is_interior(alpha, beta, rec)) return false;

        rec.t = t;
        rec.p = intersection;
        rec.mat = mat;
        rec.set_face_normal(r, normal);
        rec.object_id = object_id;
        rec.object_color = object_color;
        return true;
    }

    bool is_interior(double a, double b, hit_record& rec) const {
        interval unit_interval(0, 1);
        if (!unit_interval.contains(a) || !unit_interval.contains(b))
            return false;
        rec.u = a;
        rec.v = b;
        return true;
    }

private:
    vec3 Q;
    vec3 u, v, w;
    shared_ptr<material> mat;
    aabb bbox;
    vec3 normal;
    double D;
    int object_id;
    color object_color;
    inline static std::atomic<uint64_t> rng_state = 0x1394418719732717ULL;
};

inline shared_ptr<hittable_list> box(const vec3& a, const vec3& b, shared_ptr<material> mat)
{
    int object_id;
    color object_color;
    static std::atomic<uint64_t> rng_state = 0xDEA32758EF123456ULL;

    auto sides = make_shared<hittable_list>();

    auto min = vec3(fmin(a.x(), b.x()), fmin(a.y(), b.y()), fmin(a.z(), b.z()));
    auto max = vec3(fmax(a.x(), b.x()), fmax(a.y(), b.y()), fmax(a.z(), b.z()));

    auto dx = vec3(max.x() - min.x(), 0, 0);
    auto dy = vec3(0, max.y() - min.y(), 0);
    auto dz = vec3(0, 0, max.z() - min.z());

    sides->add(make_shared<quad>(vec3(min.x(), min.y(), max.z()), dx, dy, mat)); // front
    sides->add(make_shared<quad>(vec3(max.x(), min.y(), max.z()), -dz, dy, mat)); // right
    sides->add(make_shared<quad>(vec3(max.x(), min.y(), min.z()), -dx, dy, mat)); // back
    sides->add(make_shared<quad>(vec3(min.x(), min.y(), min.z()), dz, dy, mat)); // left
    sides->add(make_shared<quad>(vec3(min.x(), max.y(), max.z()), dx, -dz, mat)); // top
    sides->add(make_shared<quad>(vec3(min.x(), min.y(), min.z()), dx, dz, mat)); // bottom

    uint64_t local_state = rng_state.load();
    uint64_t rnd = xorshift64(local_state);
    rng_state.store(local_state);
    object_id = static_cast<int>(rnd % 256);

    float j = ((rnd >> 16) & 0xFF) / 255.0f;
    float k = ((rnd >> 8) & 0xFF) / 255.0f;
    float l = ((rnd & 0xFF)) / 255.0f;
    object_color = color(j, k, l);

    return sides;
}

#endif
