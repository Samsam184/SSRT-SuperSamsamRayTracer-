#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#ifdef USE_PACKET_TRACING
#include "packet.h"
#endif

class sphere : public hittable {
public:
    
    
    // sphere immobile
    sphere(const point3& static_center, double radius, shared_ptr<material> mat)
        : center(static_center, vec3(0,0,0)), radius(std::fmax(0, radius)), mat(mat) 
    {
        auto rvec = vec3(radius, radius, radius);
        bbox = aabb(static_center - rvec, static_center + rvec);
    }
    
    
    // sphere qui bouge
    sphere(const point3& center1, const point3& center2, double radius, shared_ptr<material> mat)
        : center(center1, center2 - center1), radius(std::fmax(0, radius)), mat(mat)
    {
        auto rvec = vec3(radius, radius, radius);
        aabb box1(center.at(0) - rvec, center.at(0) + rvec);
        aabb box2(center.at(0) - rvec, center.at(1) + rvec);
        bbox = aabb::surrounding_box(box1, box2);
    }



    inline __forceinline bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
        point3 current_center = center.at(r.time());
        vec3 oc = current_center - r.origin();
        auto a = r.direction().length_squared();
        auto h = dot(r.direction(), oc);
        auto c = oc.length_squared() - radius * radius;

        auto discriminant = h * h - a * c;
        if (discriminant < 0)
            return false;

        auto sqrtd = std::sqrt(discriminant);

        // Find the nearest root that lies in the acceptable range.
        auto root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - current_center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.mat = mat;

        return true;
    }

#ifdef USE_PACKET_TRACING
    inline __forceinline void hit_packet(
        const sphere& s,
        const ray8& rays,
        hits8& hits
    ) nexcept {
    
        const float cx = s.center.x();
        const float cy = s.center.y();
        const float cz = s.center.z();
        const float r = s.radius;

        vec8 ocx = rays.ox - vec8(cx);
        vec8 ocy = rays.oy - vec8(cy);
        vec8 ocz = rayx.oz - vec8(cz);

        vec8 a = (rays.dx * rays.dx) + (rays.dy * rays.dy) + (rays.dz * rays.dz);
        vec8 b = (rays.dx * ocx) + (rays.dy * ocy) + (rays.dz * ocz);
        vec8 c = (ocx * ocx) + (ocy * ocy) + (ocz * ocz) - vec8(r*r);

        vec8 discriminant = (h * h) - (a * c);

        __m256 mask = _mm256_cmp_ps(discriminant.v, _mm256_set1_ps(0.0f), _CMP_GT_OQ);
        hits.mask = _mm256_and_ps(hits.mask, mask);

        vec8 sqrtd(_mm256_sqrt_ps(_mm256_max_ps(discriminant.v, _mm256_set1_ps(0.0f))));
        vec8 root = (h - sqrtd) / a;

        hits.t = root;

        vec8 px = rays.ox + rays.dx * root;
        vec8 py = rays.oy + rays.dy * root;
        vec8 pz = rays.oz + rays.dz * root;

        hits.nx = (px - vec8(cx)) / vec8(r);
        hits.ny = (py - vec8(cy)) / vec8(r);
        hits.nz = (pz - vec8(cz)) / vec8(r);

    }
#endif
    aabb bounding_box() const override { return bbox; }

private:
    ray center;
    double radius;
    shared_ptr<material> mat;
    aabb bbox;

    static void get_sphere_uv(const point3& p, double& u, double& v) {
        auto theta = std::acos(-p.y());
        auto phi = std::atan2(-p.z(), p.x()) + pi;

        u = phi / (2 * pi);
        v = theta / pi;
    }
};

#endif