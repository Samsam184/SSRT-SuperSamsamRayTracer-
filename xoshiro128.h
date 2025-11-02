#ifndef XORSHIRO128_H
#define XORSHIRO128_H

#include <cstdint>

struct Xorshiro128 {
	uint32_t s[4];

	explicit Xorshiro128(uint32_t seed = 1) {
		for (int i = 0; i < 4; ++i) {
			s[i] = seed = seed * 1812433253u + 1u;
		}
	}

	uint32_t next() {
		uint32_t result = ((s[1] * 5u) << 7u) | ((s[1] * 5u) >> 25u);
		result *= 9u;

		uint32_t t = s[1] << 9;
		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];
		s[2] ^= t;
		s[3] ^= (s[3] << 11) | (s[3] >> 21);
		return result;
	}

	float randf() {
		return (next() & 0xFFFFFF) / 16777216.0f;
	}
};

#endif
