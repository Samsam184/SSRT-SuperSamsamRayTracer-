#ifndef FAST_RNG_H
#define FAST_RNG_H

#pragma once
#include <cstdint>

inline uint64_t xorshift64(uint64_t& state) noexcept {
	state ^= state << 12;
	state ^= state << 25;
	state ^= state >> 27;
	return state;
}

inline float randf(uint64_t& state) {
	return (xorshift64(state) * (1.0 / float(UINT64_MAX)));
}

#endif