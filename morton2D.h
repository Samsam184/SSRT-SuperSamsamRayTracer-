#ifndef MORTON2D_H
#define MORTON2D_H

#include <cstdint>

inline uint32_t morton2D(uint32_t x, uint32_t y) noexcept {

	uint32_t z = 0;
	for (uint32_t i = 0; i < (sizeof(uint32_t) * 8) / 2; ++i) {
		z |= ((x & (1 << i)) << i) | ((y & (1 << i)) << (i + 1));
	}
	return z;
}

#endif