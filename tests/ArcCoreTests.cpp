#include "../src/ArcCore.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

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

	std::cout << "All ARC core tests passed\n";
	return 0;
}
