#ifndef VEC3_H
#define VEC3_H

#ifndef USE_SIMD
#include "simd_vec3.h"
#endif

class vec3 {

	public: 
		double e[3];
		inline vec3() noexcept : e{0,0,0} {}
		inline vec3(double e0, double e1, double e2) noexcept : e{e0, e1, e2} {}

		inline double x() const noexcept { return e[0]; }
		inline double y() const noexcept { return e[1]; }
		inline double z() const noexcept { return e[2]; }

		inline vec3 operator-() const noexcept { return vec3(-e[0], -e[1], -e[2]); }
		inline double operator[](int i) const noexcept { return e[i]; }
		inline double& operator[](int i) noexcept { return e[i]; }

		inline vec3& operator += (const vec3& v) noexcept {
			e[0] += v.e[0];
			e[1] += v.e[1];
			e[2] += v.e[2];
			return *this;

		}

		inline vec3& operator *= (double t) noexcept {
			e[0] *= t;
			e[1] *= t;
			e[2] *= t;
			return *this;
		}
		
		inline vec3& operator/=(double t) noexcept {
			return *this *= 1 / t;
		}

		inline __forceinline double length() const noexcept {
		
		#ifdef USE_SIMD
			float af[4] = { static_cast<float>(v.e[0]), static_cast<float>(v.e[1]), static_cast<float>(v.e[2]), 0.0f };
			return static_cast<double>(simd_lenght3f(af));
		#else
			return std::sqrt(length_squared());
		#endif		
		}

		inline __forceinline double length_squared() const noexcept {
		#ifdef USE_SIMD
			float af[4] = { static_cast<float>(v.e[0]), static_cast<float>(v.e[1]), static_cast<float>(v.e[2]), 0.0f };
			return static_cast<double>(simd_dot3f(af, af));
		#else
			return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
		#endif
		}

		inline bool near_zero() const noexcept {
			auto s = 1e-8;
			return (std::fabs(e[0]) < s) && (std::fabs(e[1]) < s) && (std::fabs(e[2]) < s);
		}

		inline __forceinline static vec3 random() noexcept {
			return vec3(random_double(), random_double(), random_double());
		}

		inline __forceinline static vec3 random(double min, double max) noexcept {
			return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
		}
};

//using vec3 = vec3;

inline std::ostream& operator << (std::ostream& out, const vec3& v) noexcept{
	return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3& u, const vec3& v) noexcept{
	return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator-(const vec3& u, const vec3& v) noexcept {
	return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

inline vec3 operator*(const vec3& u, const vec3& v) noexcept {
	return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(double t, const vec3& v) noexcept {
	return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

inline vec3 operator*(const vec3& v, double t) noexcept {
	return t * v;
}

inline vec3 operator/(const vec3& v, double t) noexcept {
	return (1 / t) * v;
}

inline bool operator==(const vec3& a, const vec3& b) {
	const double eps = 1e-6;
	return fabs(a.x() - b.x()) < eps &&
		fabs(a.y() - b.y()) < eps &&
		fabs(a.z() - b.z()) < eps;
}

inline bool operator!=(const vec3& a, const vec3& b) {
	return !(a == b);
}

inline __forceinline double dot(const vec3& u, const vec3& v) noexcept {
#ifdef USE_SIMD
	float af[4] = { static_cast<float>(u.e[0]), static_cast<float>(u.e[1]), static_cast<float>(u.e[2]), 0.0f };
	float bf[4] = { static_cast<float>(v.e[0]), static_cast<float>(v.e[1]), static_cast<float>(v.e[2]), 0.0f };
	float r = simd_dot3f(af, bf);
	return static_cast<double>(r);
#else
	return u.e[0] * v.e[0]+
		   u.e[1] * v.e[1]+
		   u.e[2] * v.e[2];
#endif
}

inline __forceinline vec3 cross(const vec3& u, const vec3& v) noexcept {
#ifdef USE_SIMD
	float af[4] = { static_cast<float>(u.e[0]), static_cast<float>(u.e[1]), static_cast<float>(u.e[2]), 0.0f };
	float bf[4] = { static_cast<float>(v.e[0]), static_cast<float>(v.e[1]), static_cast<float>(v.e[2]), 0.0f };
	float out[4];
	simd_cross3f(af, bf, out);
	return vec3(static_cast<double>(out[0]), static_cast<double>(out[1]), static_cast<double>(out[2]));
#else
	return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
				u.e[2] * v.e[0] - u.e[0] * v.e[2],
				u.e[0] * v.e[1] - u.e[1] * v.e[0]);
#endif
}

inline __forceinline vec3 unit_vector(const vec3& v) noexcept {
#ifdef USE_SIMD
	float af[4] = { static_cast<float>(v.e[0]), static_cast<float>(v.e[1]), static_cast<float>(v.e[2]), 0.0f };
	float out[4];
	simd_normalize3f(af, out);
	return vec3(static_cast<double>(out[0]), static_cast<double>(out[1]), static_cast<double>(out[2]));
#else
	return v / v.length();
#endif
}

inline vec3 random_in_unit_disk() noexcept {
	while (true) {
		auto p = vec3(random_double(-1, 1), random_double(-1, 1), 0);
		if (p.length_squared() < 1) {
			return p;
		}
	}
}

inline vec3 random_unit_vector() noexcept {
	while (true) {
		auto p = vec3::random(-1, 1);
		auto lensq = p.length_squared();
		if (1e-160 < lensq && lensq <= 1) {
			return p / sqrt(lensq);
		}
	}
}

inline vec3 random_on_hemisphere(const vec3& normal) noexcept {
	vec3 on_unit_sphere = random_unit_vector();
	if (dot(on_unit_sphere, normal) > 0.0) {
		return on_unit_sphere;
	}
	else {
		return -on_unit_sphere;
	}
}

inline vec3 reflect(const vec3& v, const vec3& n) noexcept {
	return v - 2 * dot(v, n) * n;
}

inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) noexcept {
	auto cos_theta = std::fmin(dot(-uv, n), 1.0);
	vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
	vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
	return r_out_perp + r_out_parallel;
}

inline bool near_zero(const vec3& v) noexcept {
	const auto s = 1e-8;
	return (fabs(v.x()) < s) && (fabs(v.y()) < s) && (fabs(v.z()) < s);
}

#endif