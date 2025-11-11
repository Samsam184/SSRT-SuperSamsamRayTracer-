#ifndef HITTABLE_H
#define HITTABLE_H

#include "aabb.h"
#include "ray.h"
#include <memory>

class material;

class hit_record {
public:
    point3 p;
    vec3 normal;
    shared_ptr<material> mat;
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
        //normal = unit_vector(normal);
    }
};

class hittable {
public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept = 0;
    virtual aabb bounding_box() const = 0;
};

//
// --- Classe Translate : déplace un objet dans l’espace ---
//
class translate : public hittable {
public:
    translate(shared_ptr<hittable> object, const vec3& offset)
        : object(object), offset(offset)
    {
        bbox = aabb(
            object->bounding_box().axis_interval(0) + offset.x(),
            object->bounding_box().axis_interval(1) + offset.y(),
            object->bounding_box().axis_interval(2) + offset.z()
        );
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
        ray offset_r(r.origin() - offset, r.direction(), r.time());

        if (!object->hit(offset_r, ray_t, rec))
            return false;

        rec.p += offset;
        rec.set_face_normal(offset_r, rec.normal);

        return true;
    }

    aabb bounding_box() const override { return bbox; }

private:
    shared_ptr<hittable> object;
    vec3 offset;
    aabb bbox;
};

//
// --- Classe Rotate_Y : rotation autour de l’axe Y ---
//
class rotate_y : public hittable {
public:
    rotate_y(shared_ptr<hittable> object, double angle)
        : object(object)
    {
        auto radians = degrees_to_radians(angle);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
        bbox = object->bounding_box();

        point3 min(+infinity, +infinity, +infinity);
        point3 max(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
                    auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
                    auto z = k * bbox.z.max + (1 - k) * bbox.z.min;

                    auto newx = cos_theta * x + sin_theta * z;
                    auto newz = -sin_theta * x + cos_theta * z;

                    vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {
                        min[c] = std::fmin(min[c], tester[c]);
                        max[c] = std::fmax(max[c], tester[c]);
                    }
                }
            }
        }

        bbox = aabb(min, max);
    }

    inline bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
        // Transformation du rayon dans l’espace objet
        auto origin = point3(
            cos_theta * r.origin().x() - sin_theta * r.origin().z(),
            r.origin().y(),
            sin_theta * r.origin().x() + cos_theta * r.origin().z()
        );

        auto direction = vec3(
            cos_theta * r.direction().x() - sin_theta * r.direction().z(),
            r.direction().y(),
            sin_theta * r.direction().x() + cos_theta * r.direction().z()
        );

        ray rotated_r(origin, direction, r.time());

        if (!object->hit(rotated_r, ray_t, rec))
            return false;

        // Re-transformer le hit point dans l’espace monde
        rec.p = point3(
            cos_theta * rec.p.x() + sin_theta * rec.p.z(),
            rec.p.y(),
            -sin_theta * rec.p.x() + cos_theta * rec.p.z()
        );

        rec.normal = vec3(
            cos_theta * rec.normal.x() + sin_theta * rec.normal.z(),
            rec.normal.y(),
            -sin_theta * rec.normal.x() + cos_theta * rec.normal.z()
        );

        rec.set_face_normal(rotated_r, rec.normal);
        return true;
    }

    aabb bounding_box() const override { return bbox; }

private:
    shared_ptr<hittable> object;
    double sin_theta;
    double cos_theta;
    aabb bbox;
};

#endif
