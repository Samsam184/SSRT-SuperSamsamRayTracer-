#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "hittable.h"
#include "material.h"
#include "texture.h"

class constant_medium : public hittable {
public:
	
	constant_medium(shared_ptr<hittable> boundary, double density, shared_ptr<texture> tex) : boundary(boundary), neg_inv_density(-1 / density), phase_function(make_shared<isotropic>(tex)){
	
		uint64_t local_state = rng_state.load();
		uint64_t rnd = xorshift64(local_state);
		rng_state.store(local_state);
		object_id = static_cast<int>(rnd % 256);

		float r = ((rnd >> 16) & 0xFF) / 255.0f;
		float g = ((rnd >> 8) & 0xFF) / 255.0f;
		float b = ((rnd & 0xFF)) / 255.0f;
		object_color = color(r, g, b);
	}

	constant_medium(shared_ptr<hittable> boundary, double density, const color& albedo) : boundary(boundary), neg_inv_density(-1/density), phase_function(make_shared<isotropic>(albedo)) {
		uint64_t local_state = rng_state.load();
		uint64_t rnd = xorshift64(local_state);
		rng_state.store(local_state);
		object_id = static_cast<int>(rnd % 256);

		float r = ((rnd >> 16) & 0xFF) / 255.0f;
		float g = ((rnd >> 8) & 0xFF) / 255.0f;
		float b = ((rnd & 0xFF)) / 255.0f;
		object_color = color(r, g, b);
	}

	inline color get_base_color(double u, double v, const vec3& p) const noexcept {
		return phase_function ? phase_function->get_base_color(u, v, p) : color(1, 1, 1);
	}

	inline __forceinline bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
		hit_record rec1, rec2;

		if (!boundary->hit(r, interval::universe, rec1)) {
			return false;
		}


		if (!boundary->hit(r, interval(rec1.t + 0.0001, infinity), rec2)) {
			return false;
		}

		if (rec1.t < ray_t.min) {
			rec1.t = ray_t.min;
		}
		if (rec2.t > ray_t.max) {
			rec2.t = ray_t.max;
		}

		if (rec1.t >= rec2.t) {
			return false;
		}

		if (rec1.t < 0) {
			rec1.t = 0;
		}

		auto ray_lenght = r.direction().length();
		auto distance_inside_boundary = (rec2.t - rec1.t) * ray_lenght;
		auto hit_distance = neg_inv_density * std::log(random_double());

		if (hit_distance > distance_inside_boundary) {
			return false;
		}

		rec.t = rec1.t + hit_distance / ray_lenght;
		rec.p = r.at(rec.t);

		rec.normal = vec3(1, 0, 0);
		rec.front_face = true;
		rec.mat = phase_function;
		rec.object_id = object_id;

		return true;

	}

	

	aabb bounding_box() const override { return boundary->bounding_box(); }

private:
	shared_ptr<hittable> boundary;
	double neg_inv_density;
	shared_ptr<material> phase_function;
	int object_id;
	color object_color;
	inline static std::atomic<uint64_t> rng_state = 0xDEADBE257345678ULL;
};

#endif