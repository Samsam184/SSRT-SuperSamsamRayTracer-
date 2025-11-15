#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"
#include "interval.h"
#include "aabb.h"
#include <memory>


class triangle : public hittable {
public:
	vec3 v0, v1, v2;
	vec3 n0, n1, n2;
	vec3 uv0, uv1, uv2;
	std::shared_ptr<material> mat;
	aabb bbox;

	triangle() {}
	triangle(
		const vec3& a, const vec3& b, const vec3& c,
		const vec3& na, const vec3& nb, const vec3& nc,
		const vec3& uva, const vec3& uvb, const vec3& uvc,
		std::shared_ptr<material> m
	) : v0(a), v1(b), v2(c),
		n0(na), n1(nb), n2(nc),
		uv0(uva), uv1(uvb), uv2(uvc),
		mat(m) 
	{
		vec3 minp(
			std::min({ v0.x(), v1.x(), v2.x() }),
			std::min({ v0.y(), v1.y(), v2.y() }),
			std::min({ v0.z(), v1.z(), v2.z() })
		);

		vec3 maxp(
			std::max({ v0.x(), v1.x(), v2.x() }),
			std::max({ v0.y(), v1.y(), v2.y() }),
			std::max({ v0.z(), v1.z(), v2.z() })
		);

		bbox = aabb(minp, maxp);
	}

	bool hit(const ray& r, interval ray_t, hit_record& rec) const noexcept override {
		const double EPSILON = 1e-8;
		vec3 edge1 = v1 - v0;
		vec3 edge2 = v2 - v0;
		vec3 h = cross(r.direction(), edge2);
		double a = dot(edge1, h);

		if (fabs(a) < EPSILON) return false;

		double f = 1.0 / a;
		vec3 s = r.origin() - v0;
		double u = f * dot(s, h);
		if (u < 0.0 || u > 1.0) return false;

		vec3 q = cross(s, edge1);
		double v = f * dot(r.direction(), q);
		if (v < 0.0 || u + v > 1.0) return false;

		double t = f * dot(edge2, q);
		if (!ray_t.surrounds(t)) return false;

		rec.t = t;
		rec.p = r.at(t);

		double w = 1.0 - u - v;
		rec.normal = unit_vector(w * n0 + u * n1 + v * n2);
		rec.u = w * uv0.x() + u * uv1.x() + v * uv2.x();
		rec.v = w * uv0.y() + u * uv1.y() + v * uv2.y();
		rec.mat = mat;
		rec.set_face_normal(r, rec.normal);

		return true;

	}

	aabb bounding_box() const override { return bbox; }
};


#endif