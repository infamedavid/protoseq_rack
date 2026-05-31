#include "../src/ArcCore.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace protoseq;

namespace {

bool approx(float a, float b, float eps = 1e-6f) {
	return std::fabs(a - b) <= eps;
}

void assertMultiplier(int index, float label, int numerator, int denominator) {
	const ArcMultiplier multiplier = arcMultiplierForIndex(index);
	assert(approx(multiplier.label, label));
	assert(multiplier.numerator == numerator);
	assert(multiplier.denominator == denominator);
}

} // namespace

int main() {
	static_assert(ARC_MULTIPLIERS.size() == 21, "ARC multiplier table must have exactly 21 entries");
	assert(ARC_MULTIPLIERS.size() == 21);

	assertMultiplier(0, 1.0f, 1, 1);
	assertMultiplier(1, 1.5f, 3, 2);
	assertMultiplier(3, 2.5f, 5, 2);
	assertMultiplier(5, 3.5f, 7, 2);
	assertMultiplier(ARC_MULTIPLIER_DEFAULT_INDEX, 4.0f, 4, 1);
	assertMultiplier(20, 32.0f, 32, 1);

	assert(arcMultiplierIndexFromParam(-10.0f) == 0);
	assert(arcMultiplierIndexFromParam(0.49f) == 0);
	assert(arcMultiplierIndexFromParam(0.50f) == 1);
	assert(arcMultiplierIndexFromParam(20.0f) == 20);
	assert(arcMultiplierIndexFromParam(30.0f) == 20);

	assert(arcMultiplierIndexFromNormalized(-1.0f) == 0);
	assert(arcMultiplierIndexFromNormalized(0.0f) == 0);
	assert(arcMultiplierIndexFromNormalized(0.5f) == 10);
	assert(arcMultiplierIndexFromNormalized(1.0f) == 20);
	assert(arcMultiplierIndexFromNormalized(2.0f) == 20);

	assert(arcBarLengthFromParam(-10.0f) == 1);
	assert(arcBarLengthFromParam(4.4f) == 4);
	assert(arcBarLengthFromParam(4.5f) == 5);
	assert(arcBarLengthFromParam(100.0f) == 64);
	assert(arcBarLengthFromNormalized(0.0f) == 1);
	assert(arcBarLengthFromNormalized(1.0f) == 64);
	assert(arcRatchetCountFromParam(-10.0f) == 1);
	assert(arcRatchetCountFromParam(1.4f) == 1);
	assert(arcRatchetCountFromParam(1.5f) == 2);
	assert(arcRatchetCountFromParam(100.0f) == 8);
	assert(arcRatchetCountFromNormalized(0.0f) == 1);
	assert(arcRatchetCountFromNormalized(1.0f) == 8);

	assert(arcSeedBucketFromNormalized(-1.0f) == 0);
	assert(arcSeedBucketFromNormalized(0.0f) == 0);
	assert(arcSeedBucketFromNormalized(0.0001f) == 1);
	assert(arcSeedBucketFromNormalized(0.5f) == 500);
	assert(arcSeedBucketFromNormalized(1.0f) == 1000);
	assert(arcSeedBucketFromNormalized(2.0f) == 1000);

	const std::uint64_t brnlA = buildArcSeed(123, 2, 8, ArcRandomChannelId::BRNL);
	const std::uint64_t brnlB = buildArcSeed(123, 2, 8, ArcRandomChannelId::BRNL);
	const std::uint64_t rlen = buildArcSeed(123, 2, 8, ArcRandomChannelId::RLEN);
	assert(brnlA == brnlB);
	assert(brnlA != rlen);

	assert(arcBarStep(0, 4) == 0);
	assert(arcBarStep(3, 4) == 3);
	assert(arcBarStep(4, 4) == 0);
	assert(arcBarStep(9, 4) == 1);
	assert(arcBarStep(-1, 4) == 3);

	assert(!arcShouldSkipBernoulli(0.0f, 0.0));
	assert(!arcShouldSkipBernoulli(0.0f, 0.999));
	assert(arcShouldSkipBernoulli(1.0f, 0.0));
	assert(arcShouldSkipBernoulli(1.0f, 0.999));
	assert(arcShouldSkipBernoulli(0.5f, 0.25));
	assert(!arcShouldSkipBernoulli(0.5f, 0.75));

	const bool fixedSkipA = arcShouldSkipBernoulliDeterministic(321, 3, 8, 0.5f, ArcRandomChannelId::BRNL);
	const bool fixedSkipB = arcShouldSkipBernoulliDeterministic(321, 3, 8, 0.5f, ArcRandomChannelId::BRNL);
	assert(fixedSkipA == fixedSkipB);
	assert(arcShouldSkipBernoulliDeterministic(321, 3, 8, 0.0f, ArcRandomChannelId::BRNL) == false);
	assert(arcShouldSkipBernoulliDeterministic(321, 3, 8, 1.0f, ArcRandomChannelId::BRNL) == true);
	assert(arcUnitRandomFromSeed(buildArcSeed(321, 3, 8, ArcRandomChannelId::BRNL)) != arcUnitRandomFromSeed(buildArcSeed(321, 3, 8, ArcRandomChannelId::RLEN)));

	const ArcMultiplier oneX = arcMultiplierForIndex(0);
	const ArcMultiplier onePointFiveX = arcMultiplierForIndex(1);
	assert(approx(static_cast<float>(arcStepDurationSeconds(120.0f, oneX)), 0.5f));
	assert(approx(static_cast<float>(arcStepDurationSeconds(120.0f, onePointFiveX)), 1.0f / 3.0f));
	assert(approx(static_cast<float>(arcEffectiveGateDurationSeconds(10.0, 10.5, 0.4, 0.01)), 0.4f));
	assert(approx(static_cast<float>(arcEffectiveGateDurationSeconds(10.0, 10.5, 0.5, 0.01)), 0.49f));
	assert(approx(static_cast<float>(arcEffectiveGatePhase(1.0f, 10.0, 10.5, 0.01)), 0.98f));
	assert(approx(static_cast<float>(arcEffectiveGatePhase(0.5f, 10.0, 10.5, 0.01)), 0.5f));

	assert(approx(arcApplyRandomLengthShortening(0.75f, 0.0f, 0.9), 0.75f));
	assert(approx(arcApplyRandomLengthShortening(0.75f, 1.0f, 0.25), 0.5625f));
	assert(arcApplyRandomLengthShortening(0.75f, 1.0f, 0.25) <= 0.75f);
	assert(arcApplyRandomLengthShortening(0.75f, 0.5f, 1.0) <= 0.75f);
	const float rlenA = arcApplyRandomLengthShorteningDeterministic(123, 2, 8, 0.75f, 0.5f, ArcRandomChannelId::RLEN);
	const float rlenB = arcApplyRandomLengthShorteningDeterministic(123, 2, 8, 0.75f, 0.5f, ArcRandomChannelId::RLEN);
	const float rlenDifferentSeed = arcApplyRandomLengthShorteningDeterministic(124, 2, 8, 0.75f, 0.5f, ArcRandomChannelId::RLEN);
	const float brnlChannelLength = arcApplyRandomLengthShorteningDeterministic(123, 2, 8, 0.75f, 0.5f, ArcRandomChannelId::BRNL);
	assert(approx(rlenA, rlenB));
	assert(!approx(rlenA, rlenDifferentSeed));
	assert(!approx(rlenA, brnlChannelLength));

	assert(!arcShouldRatchet(1.0f, 0.0, 1));
	assert(!arcShouldRatchet(0.0f, 0.0, 4));
	assert(arcShouldRatchet(1.0f, 0.999, 4));
	const bool fixedRatchetA = arcShouldRatchetDeterministic(321, 3, 8, 0.5f, 4, ArcRandomChannelId::RATCH);
	const bool fixedRatchetB = arcShouldRatchetDeterministic(321, 3, 8, 0.5f, 4, ArcRandomChannelId::RATCH);
	assert(fixedRatchetA == fixedRatchetB);
	assert(arcUnitRandomFromSeed(buildArcSeed(321, 3, 8, ArcRandomChannelId::RATCH)) != arcUnitRandomFromSeed(buildArcSeed(321, 3, 8, ArcRandomChannelId::BRNL)));
	assert(arcUnitRandomFromSeed(buildArcSeed(321, 3, 8, ArcRandomChannelId::RATCH)) != arcUnitRandomFromSeed(buildArcSeed(321, 3, 8, ArcRandomChannelId::RLEN)));
	assert(arcRatchetGateActive(0.05, 0.6, 1, 0.01));
	assert(!arcRatchetGateActive(0.61, 0.6, 1, 0.01));
	assert(arcRatchetGateActive(0.05, 0.6, 3, 0.01));
	assert(!arcRatchetGateActive(0.195, 0.6, 3, 0.01));
	assert(arcRatchetGateActive(0.21, 0.6, 3, 0.01));

	assert(approx(static_cast<float>(arcSwingDelaySeconds(0.5, 0.0f)), 0.0f));
	assert(approx(static_cast<float>(arcSwingDelaySeconds(0.5, 1.0f)), 0.25f));
	assert(!arcShouldSwing(0.0f, 0.0));
	assert(arcShouldSwing(1.0f, 0.999));
	const bool fixedSwingA = arcShouldSwingDeterministic(321, 3, 8, 0.5f, ArcRandomChannelId::SWNG);
	const bool fixedSwingB = arcShouldSwingDeterministic(321, 3, 8, 0.5f, ArcRandomChannelId::SWNG);
	assert(fixedSwingA == fixedSwingB);
	assert(arcUnitRandomFromSeed(buildArcSeed(321, 3, 8, ArcRandomChannelId::SWNG)) != arcUnitRandomFromSeed(buildArcSeed(321, 3, 8, ArcRandomChannelId::BRNL)));
	const std::vector<ArcSwingScheduleEvent> noSwingSchedule = arcBuildSwingSchedule(4, 10.0, 0.5, 1.0f, 0.0f, 123);
	assert(noSwingSchedule.size() == 5);
	assert(approx(static_cast<float>(noSwingSchedule[0].start), 10.0f));
	assert(approx(static_cast<float>(noSwingSchedule[4].start), 12.0f));
	const std::vector<ArcSwingScheduleEvent> fullSwingSchedule = arcBuildSwingSchedule(4, 10.0, 0.5, 1.0f, 1.0f, 123);
	assert(fullSwingSchedule.size() == 5);
	assert(fullSwingSchedule[4].barStep == 0);
	assert(approx(static_cast<float>(fullSwingSchedule[4].start), 12.25f));
	assert(arcEffectiveGateDurationSeconds(fullSwingSchedule[3].start, fullSwingSchedule[4].start, 0.5, 0.01) <= (fullSwingSchedule[4].start - fullSwingSchedule[3].start));

	std::cout << "All ARC core tests passed\n";
	return 0;
}
