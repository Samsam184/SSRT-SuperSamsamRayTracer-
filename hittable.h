#ifndef HITTABLE_H
#define HITTABLE_H

#include "aabb.h"
#include "ray.h"
#include <memory>

class material;

class hit_record {
public:
    vec3 p;
    vec3 normal;
    std::shared_ptr<material> mat;
    double t;
    double u;
    double v;
    bool front_face;

    color albedo;
    float roughness = 0.0f;
    float metallic = 0.0f;
    color emission = color(0, 0, 0);
    int object_id = 0;
    color object_color = color(0, 0, 0);

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// =====================================================
// Base Hittable
// =====================================================

class hittable {
public:
    virtual ~hittable() = default;
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept = 0;
    virtual aabb bounding_box() const = 0;
};

// =====================================================
// TRANSLATE
// =====================================================

class translate : public hittable {
public:
    std::shared_ptr<hittable> ptr;
    vec3 offset;

    translate(std::shared_ptr<hittable> p, const vec3& displacement)
        : ptr(p), offset(displacement) {
    }

    bool hit(const ray& r, interval t_range, hit_record& rec) const noexcept override {
        ray moved_r(r.origin() - offset, r.direction(), r.time());

        if (!ptr->hit(moved_r, t_range, rec)) return false;

        rec.p += offset;
        rec.set_face_normal(moved_r, rec.normal);
        return true;
    }

    aabb bounding_box() const override {
        aabb box = ptr->bounding_box();
        return aabb(box.min() + offset, box.max() + offset);
    }
};

// =====================================================
// ROTATE Y
// =====================================================

class rotate_y : public hittable {
public:
    std::shared_ptr<hittable> ptr;
    double sin_theta;
    double cos_theta;
    aabb bbox;

    rotate_y(std::shared_ptr<hittable> p, double angle_deg) : ptr(p) {

        double radians = angle_deg * pi / 180.0;
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);

        aabb b = ptr->bounding_box();

        vec3 minp(infinity, infinity, infinity);
        vec3 maxp(-infinity, -infinity, -infinity);

        // Rotate the 8 corners
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    double x = i ? b.max().x() : b.min().x();
                    double y = j ? b.max().y() : b.min().y();
                    double z = k ? b.max().z() : b.min().z();

                    double newx = cos_theta * x + sin_theta * z;
                    double newz = -sin_theta * x + cos_theta * z;

                    vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {
                        minp.e[c] = std::min(minp.e[c], tester.e[c]);
                        maxp.e[c] = std::max(maxp.e[c], tester.e[c]);
                    }
                }
            }
        }

        bbox = aabb(minp, maxp);
    }

    bool hit(const ray& r, interval t_range, hit_record& rec) const noexcept override {
        vec3 origin = r.origin();
        vec3 direction = r.direction();

        // Inverse rotation to bring ray into object space
        origin.e[0] = cos_theta * r.origin().x() - sin_theta * r.origin().z();
        origin.e[2] = sin_theta * r.origin().x() + cos_theta * r.origin().z();

        direction.e[0] = cos_theta * r.direction().x() - sin_theta * r.direction().z();
        direction.e[2] = sin_theta * r.direction().x() + cos_theta * r.direction().z();

        ray rotated_r(origin, direction, r.time());

        if (!ptr->hit(rotated_r, t_range, rec)) return false;

        // Rotate hit point + normal back to world
        vec3 p = rec.p;
        vec3 normal = rec.normal;

        p.e[0] = cos_theta * rec.p.x() + sin_theta * rec.p.z();
        p.e[2] = -sin_theta * rec.p.x() + cos_theta * rec.p.z();

        normal.e[0] = cos_theta * rec.normal.x() + sin_theta * rec.normal.z();
        normal.e[2] = -sin_theta * rec.normal.x() + cos_theta * rec.normal.z();

        rec.p = p;
        rec.normal = normal;
        rec.set_face_normal(rotated_r, normal);

        return true;
    }

    aabb bounding_box() const override {
        return bbox;
    }
};

// =====================================================
// SCALE
// =====================================================

class scale : public hittable {
public:
    std::shared_ptr<hittable> ptr;
    vec3 factor;

    scale(std::shared_ptr<hittable> p, const vec3& s)
        : ptr(p), factor(s) {
    }

    bool hit(const ray& r, interval t_range, hit_record& rec) const noexcept override {

        // Transform ray to object space
        vec3 inv(
            1.0 / factor.x(),
            1.0 / factor.y(),
            1.0 / factor.z()
        );

        ray scaled_r(r.origin() * inv, r.direction() * inv, r.time());

        if (!ptr->hit(scaled_r, t_range, rec)) return false;

        // Bring hit back to world space
        rec.p = rec.p * factor;

        // Normals transform with inverse transpose
        vec3 invT = vec3(
            1.0 / factor.x(),
            1.0 / factor.y(),
            1.0 / factor.z()
        );

        rec.normal = unit_vector(rec.normal * invT);
        rec.set_face_normal(scaled_r, rec.normal);

        return true;
    }

    aabb bounding_box() const override {
        aabb b = ptr->bounding_box();
        return aabb(b.min() * factor, b.max() * factor);
    }
};

#endif
