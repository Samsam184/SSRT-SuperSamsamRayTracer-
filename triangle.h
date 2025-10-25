#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "vec3.h"
#include "hittable.h"

class triangle : public hittable {
public:
	point3 v0, v1, v2; 
	shared_ptr<material> mat_ptr;

	triangle() {}
	triangle(point3 _v0, point3 _v1, point3 _v2, shared_ptr<material> m) : v0(_v0), v1(_v1), v2(_v2), mat_ptr(m) {}

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
		// Moller-Trumbore algorithm

		//variables
		const double EPSILON = 1e-8;
		vec3 edge1 = v1 - v0;
		vec3 edge2 = v2 - v0;
		vec3 h = cross(r.direction(), edge2);
		double a = dot(edge1, h);
		//variables

		//conditions pour intersection des rays sur le triangle
		if (fabs(a) < EPSILON) return false;

		double f = 1.0 / a;
		vec3 s = r.origin() - v0;
		double u = f * dot(s, h);
		if (u < 0.0 || u > 1.0) return false;

		vec3 q = cross(s, edge1);
		double v = f * dot(r.direction(), q);
		if (v < 0.0 || u + v>1.0) return false;

		double t = f * dot(edge2, q);
		if (t < t_min || t>t_max) return false;

		rec.t = t;
		rec.p = r.at(t);
		rec.normal = unit_vector(cross(edge1, edge2));
		rec.mat_ptr = mat_ptr;

		return true;

	}
};

#endif