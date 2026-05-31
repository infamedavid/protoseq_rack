#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace protoseq {

struct ArcMultiplier {
	float label;
	int numerator;
	int denominator;
};

constexpr std::array<ArcMultiplier, 21> ARC_MULTIPLIERS{{
	{1.0f, 1, 1},
	{1.5f, 3, 2},
	{2.0f, 2, 1},
	{2.5f, 5, 2},
	{3.0f, 3, 1},
	{3.5f, 7, 2},
	{4.0f, 4, 1},
	{5.0f, 5, 1},
	{6.0f, 6, 1},
	{7.0f, 7, 1},
	{8.0f, 8, 1},
	{9.0f, 9, 1},
	{10.0f, 10, 1},
	{11.0f, 11, 1},
	{12.0f, 12, 1},
	{13.0f, 13, 1},
	{14.0f, 14, 1},
	{15.0f, 15, 1},
	{16.0f, 16, 1},
	{24.0f, 24, 1},
	{32.0f, 32, 1},
}};

constexpr int ARC_MULTIPLIER_INDEX_MIN = 0;
constexpr int ARC_MULTIPLIER_INDEX_MAX = 20;
constexpr int ARC_BAR_MIN_EVENTS = 1;
constexpr int ARC_BAR_MAX_EVENTS = 64;

inline float arcClamp01(float value) {
	return std::min(std::max(value, 0.0f), 1.0f);
}

inline int clampArcMultiplierIndex(int index) {
	return std::min(std::max(index, ARC_MULTIPLIER_INDEX_MIN), ARC_MULTIPLIER_INDEX_MAX);
}

inline int arcMultiplierIndexFromParam(float value) {
	return clampArcMultiplierIndex(static_cast<int>(std::round(value)));
}

inline int arcMultiplierIndexFromNormalized(float voltage) {
	return clampArcMultiplierIndex(static_cast<int>(std::round(arcClamp01(voltage) * ARC_MULTIPLIER_INDEX_MAX)));
}

inline ArcMultiplier arcMultiplierForIndex(int index) {
	return ARC_MULTIPLIERS[clampArcMultiplierIndex(index)];
}

inline int arcBarLengthFromParam(float value) {
	return std::min(std::max(static_cast<int>(std::round(value)), ARC_BAR_MIN_EVENTS), ARC_BAR_MAX_EVENTS);
}

inline int arcBarLengthFromNormalized(float voltage) {
	const float scaled = static_cast<float>(ARC_BAR_MIN_EVENTS) + arcClamp01(voltage) * static_cast<float>(ARC_BAR_MAX_EVENTS - ARC_BAR_MIN_EVENTS);
	return arcBarLengthFromParam(scaled);
}

} // namespace protoseq
