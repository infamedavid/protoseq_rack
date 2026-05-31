#pragma once

#include <cstdint>

namespace protoseq {

inline std::uint64_t mixSeed(std::uint64_t state, std::uint64_t value) {
	state ^= value + 0x9e3779b97f4a7c15ULL + (state << 6U) + (state >> 2U);
	return state;
}

} // namespace protoseq
