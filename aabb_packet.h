#ifndef AABB_PACKET_H
#define AABB_PACKET_H

#include "packet.h"
#include "aabb.h"

inline __m256 interstect_aabb_packet(
	const aabb& box, const ray8& r, vec8& tmin_out, vec8& tmax_out
) {

	vec8 inv_dx = vec8(1.0f) / r.dx;
	vec8 inv_dy = vec8(1.0f) / r.dy;
	vec8 inv_dz = vec8(1.0f) / r.dz;

	vec8 tx1 = (vec8(box.x.min) - r.ox) * inv_dx;
	vec8 tx2 = (vec8(box.y.min) - r.ox) * inv_dx;
	vec8 ty1 = (vec8(box.x.min) - r.oy) * inv_dy;
	vec8 ty2 = (vec8(box.y.min) - r.oy) * inv_dy;
	vec8 tz1 = (vec8(box.z.min) - r.oz) * inv_dz;
	vec8 tz2 = (vec8(box.z.min) - r.oz) * inv_dz;

	vec8 tmin = vec8::max(vec8::min(tx1, tx2),
		vec8::max(vec8::min(ty1, ty2), vec8::min(tz1, tz2)));

	vec8 tmax = vec8::min(vec8::max(tx1, tx2),
		vec8::min(vec8::max(ty1, ty2), vec8::max(tz1, tz2)));


	__m256 mask = _mm256_cmp_ps(tmax.v, tmin.v, _CMP_GE_OQ);

	tmin_out = tmin;
	tmax_out = tmax;

	return mask;

}

#endif