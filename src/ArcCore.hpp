#pragma once

#include "ProtoRandom.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

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

constexpr int ARC_SEED_BUCKET_MIN = 0;
constexpr int ARC_SEED_BUCKET_MAX = 1000;

enum class ArcRandomChannelId : int {
	BRNL = 1,
	RLEN = 2,
	RATCH = 3,
	SWNG = 4,
};

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

inline int arcSeedBucketFromNormalized(float value) {
	const float clamped = arcClamp01(value);
	if (clamped <= 0.0f) {
		return ARC_SEED_BUCKET_MIN;
	}
	return std::min(std::max(static_cast<int>(std::round(clamped * ARC_SEED_BUCKET_MAX)), 1), ARC_SEED_BUCKET_MAX);
}

inline int arcBarStep(int arcStepIndex, int barLen) {
	const int clampedBarLen = arcBarLengthFromParam(static_cast<float>(barLen));
	const int wrapped = arcStepIndex % clampedBarLen;
	return wrapped < 0 ? wrapped + clampedBarLen : wrapped;
}

inline std::uint64_t buildArcSeed(int seedBucket, int barStep, int barLen, ArcRandomChannelId channelId) {
	std::uint64_t state = 0xcbf29ce484222325ULL;
	state = mixSeed(state, static_cast<std::uint64_t>(std::min(std::max(seedBucket, ARC_SEED_BUCKET_MIN), ARC_SEED_BUCKET_MAX)));
	state = mixSeed(state, static_cast<std::uint64_t>(arcBarStep(barStep, barLen)));
	state = mixSeed(state, static_cast<std::uint64_t>(arcBarLengthFromParam(static_cast<float>(barLen))));
	state = mixSeed(state, static_cast<std::uint64_t>(static_cast<int>(channelId)));
	return state;
}

inline double arcStepDurationSeconds(float bpm, const ArcMultiplier& multiplier) {
	const float clampedBpm = std::max(bpm, 1.0f);
	return (60.0 / static_cast<double>(clampedBpm)) * static_cast<double>(multiplier.denominator) / static_cast<double>(multiplier.numerator);
}

inline double arcEffectiveGateDurationSeconds(double currentStepStart, double nextStepStart, double desiredGateDuration, double safetyGap) {
	const double available = std::max(0.0, nextStepStart - currentStepStart - std::max(safetyGap, 0.0));
	return std::min(std::max(desiredGateDuration, 0.0), available);
}

inline double arcEffectiveGatePhase(float gateLength, double currentStepStart, double nextStepStart, double safetyGap) {
	const double stepDuration = std::max(nextStepStart - currentStepStart, 0.0);
	if (stepDuration <= 0.0) {
		return 0.0;
	}
	const double desiredGateDuration = static_cast<double>(arcClamp01(gateLength)) * stepDuration;
	return arcEffectiveGateDurationSeconds(currentStepStart, nextStepStart, desiredGateDuration, safetyGap) / stepDuration;
}

inline double arcUnitRandomFromSeed(std::uint64_t seed) {
	seed ^= seed >> 30U;
	seed *= 0xbf58476d1ce4e5b9ULL;
	seed ^= seed >> 27U;
	seed *= 0x94d049bb133111ebULL;
	seed ^= seed >> 31U;
	return static_cast<double>(seed >> 11U) * (1.0 / 9007199254740992.0);
}

inline bool arcShouldSkipBernoulli(float skipProbability, double randomUnit) {
	const float clampedProbability = arcClamp01(skipProbability);
	if (clampedProbability <= 0.0f) {
		return false;
	}
	if (clampedProbability >= 1.0f) {
		return true;
	}
	return std::min(std::max(randomUnit, 0.0), 1.0) < static_cast<double>(clampedProbability);
}

inline bool arcShouldSkipBernoulliDeterministic(int seedBucket, int barStep, int barLen, float skipProbability, ArcRandomChannelId channelId = ArcRandomChannelId::BRNL) {
	return arcShouldSkipBernoulli(skipProbability, arcUnitRandomFromSeed(buildArcSeed(seedBucket, barStep, barLen, channelId)));
}

} // namespace protoseq
