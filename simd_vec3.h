#ifndef SIMD_VEC3_H
#define SIMD_VEC3_H

#include <immintrin.h>
#include <cstdint>
#include <cmath>

#if defined(__AVX__)
	#define SIMD_HAVE_AVX 1
#elif defined(_MSC_VER)
	#define SIMD_HAVE_SSE 1
#else 
	#define SIMD_HAVE_SSE 1
#endif

inline __m128 simd_set_f32(float x, float y, float z) noexcept {
	return _mm_set_ps(0.0f, z, y, x);
}

inline void simd_store_f32(__m128 v, float& x, float& y, float& z) noexcept {
	alignas(16) float tmp[4];
	_mm_store_ps(tmp, v);
	x = tmp[0];
	y = tmp[1];
	z = tmp[2];
}

inline float simd_dot3f(const float* a, const float* b) {
	__m128 va = _mm_loadu_ps(a);
	__m128 vb = _mm_loadu_ps(b);
	__m128 mul = _mm_mul_ps(va, vb);
	__m128 sum = _mm_hadd_ps(mul, mul);
	sum = _mm_hadd_ps(sum, sum);
	float dot = _mm_cvtss_f32(sum);
	if (!std::isfinite(dot)) dot = 0.0f;  // <-- sécurité
	return dot;
}

inline void simd_cross3f(const float* a, const float* b, float* out) noexcept {
	__m128 va = _mm_loadu_ps(a);
	__m128 vb = _mm_loadu_ps(b);

	__m128 a_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 b_zxy = _mm_shuffle_ps(vb, vb, _MM_SHUFFLE(3, 1, 0, 2));
	__m128 a_zxy = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 b_yzx = _mm_shuffle_ps(va, va, _MM_SHUFFLE(3, 0, 2, 1));

	__m128 c1 = _mm_mul_ps(a_yzx, b_zxy);
	__m128 c2 = _mm_mul_ps(a_zxy, b_yzx);

	__m128 vc = _mm_sub_ps(c1, c2);

	_mm_storeu_ps(out, vc);

}

inline float simd_lenght3f(const float* a) {
	__m128 v = _mm_loadu_ps(a);
	v = _mm_mul_ps(v, v);
	__m128 sum = _mm_hadd_ps(v, v);
	sum = _mm_hadd_ps(sum, sum);
	float len = std::sqrt(_mm_cvtss_f32(sum));
	if (!std::isfinite(len)) len = 0.0f;  // <-- sécurité anti-NaN
	return len;

}

inline void simd_normalize3f(const float* a, float* out) noexcept {
	__m128 va = _mm_loadu_ps(a);
	__m128 mul = _mm_mul_ps(va, va);
	__m128 t = _mm_hadd_ps(mul, mul);
	__m128 s = _mm_hadd_ps(t, t);
	float sum = _mm_cvtss_f32(s);
	if (sum == 0.0f) { out[0] = 0; out[1] = 0; out[2] = 0; return; }

	__m128 vsum = _mm_set_ss(sum);
	__m128 r = _mm_rsqrt_ss(vsum);
	__m128 r2 = _mm_mul_ss(r, r);
	__m128 tmp = _mm_mul_ss(r2, vsum);
	__m128 three = _mm_set_ss(3.0f);
	__m128 numer = _mm_sub_ss(three, tmp);
	__m128 half = _mm_set_ss(0.5f);
	__m128 rn = _mm_mul_ss(_mm_mul_ss(r, numer), half);
	float rf = _mm_cvtss_f32(rn);

	__m128 vrf = _mm_set1_ps(rf);
	__m128 vout = _mm_mul_ps(va, vrf);
	_mm_storeu_ps(out, vout);

}

#endif