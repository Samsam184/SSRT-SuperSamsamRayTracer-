#ifndef PACKET_H
#define PACKET_H

#include <immintrin.h>
#include <algorithm>

struct vec8 {
	__m256 v;

	vec8(){}
	vec8(float f) { v = _mm256_set1_ps(f); }
	vec8(__m256 vv) : v(vv){}

	inline vec8 operator+(const vec8& b) const { return _mm256_add_ps(v, b.v); }
	inline vec8 operator-(const vec8& b) const { return _mm256_sub_ps(v, b.v); }
	inline vec8 operator*(const vec8& b) const { return _mm256_mul_ps(v, b.v); }
	inline vec8 operator/(const vec8& b) const { return _mm256_div_ps(v, b.v); }

	static inline vec8 min(const vec8& a, const vec8& b) { return _mm256_min_ps(a.v, b.v); }
	static inline vec8 max(const vec8& a, const vec8& b) { return _mm256_max_ps(a.v, b.v); }

};

struct ray8 {
	vec8 ox, oy, oz;
	vec8 dx, dy, dz;
	vec8 tmin, tmax;
};

struct hit8 {
	vec8 t;
	vec8 nx, ny, nz;
	__m256 mask;
};

#endif