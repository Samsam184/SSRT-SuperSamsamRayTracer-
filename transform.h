#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "hittable.h"
#include "rtweekend.h"

class translate : public hittable {
public:
	std::shared_ptr<hittable> ptr;
	vec3 offset;

	translate(std::shared_ptr<hittable> p, const vec3& displacement) : ptr(p), offset(displacement) {}

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {

		ray moved_r(r.origin() - offset, r.direction(), r.time());

		if (!ptr->hit(moved_r, ray_t, rec)) return false;

		rec.p += offset;
		rec.set_face_normal(moved_r, rec.normal);

		return true;
	}

	virtual aabb bounding_box() const override {
		aabb box = ptr->bounding_box();
		return aabb(box.min() + offset, box.max() + offset);
	}
};

class rotate_y : public hittable {
public:
	std::shared_ptr<hittable> ptr;
	double sin_theta;
	double cos_theta;
	bool hasbox;
	aabb bbox;

	rotate_y(std::shared_ptr<hittable> p, double angle_deg) : ptr(p) {

		double radians = (3.14159265359 / 180.0) * angle_deg;
		sin_theta = std::sin(radians);
		cos_theta = std::cos(radians);
		bbox = ptr->bounding_box();
		hasbox = true;

		vec3 minp(infinity, infinity, infinity);
		vec3 maxp(-infinity, -infinity, -infinity);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				for (int k = 0; k < 2; k++) {

					double x = i ? bbox.max().x() : bbox.min().x();
					double y = j ? bbox.max().y() : bbox.min().y();
					double z = k ? bbox.max().z() : bbox.min().z();

					double newx = cos_theta * x + sin_theta * z;
					double newz = -sin_theta * x + cos_theta * z;

					vec3 tester(newx, y, newz);

					for (int c = 0; c < 3; c++) {
						minp.e[c] = std::min(minp.e[c], tester.e[c]);
						minp.e[c] = std::max(minp.e[c], tester.e[c]);

					}

				}
			}
		}

		bbox = aabb(minp, maxp);
	}

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {

		vec3 origin = r.origin();
		vec3 direction = r.direction();

		origin.e[0] = cos_theta * r.origin().x() - sin_theta * r.origin().z();
		origin.e[2] = sin_theta * r.origin().x() - cos_theta * r.origin().z();

		direction.e[0] = cos_theta * r.direction().x() - sin_theta * r.direction().z();
		direction.e[2] = sin_theta * r.direction().x() - cos_theta * r.direction().z();

		ray rotated_r(origin, direction, r.time());

		if (!ptr->hit(rotated_r, ray_t, rec)) return false;

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

	virtual aabb bounding_box() const override {
		return bbox;
	}
};

class scale : public hittable {
public:
	std::shared_ptr<hittable> ptr;
	double factor;

	scale(std::shared_ptr<hittable> p, double s) : ptr(p), factor(s) {}

	bool hit(const ray& r, interval t_range, hit_record& rec) const noexcept override {
		ray local_r(r.origin() / factor, r.direction(), r.time());

		if (!ptr->hit(local_r, t_range, rec)) return false;

		rec.p *= factor;
		rec.normal = unit_vector(rec.normal);

		return true;
	}

	aabb bounding_box() const override {
		aabb box = ptr->bounding_box();
		return aabb(box.min() * factor, box.max() * factor);
	}
};

#endif