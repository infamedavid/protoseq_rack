#pragma once

#include "ProtoRandom.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace protoseq {

struct WeightedIndex {
    std::size_t index;
    float weight;
};

std::uint64_t buildDeterministicSeed(
    int seedBucket,
    int noteIndex,
    int pivotMidi,
    int keyRootSemitone,
    int mode,
    int rangeBucket);

std::size_t chooseWeightedIndexDeterministic(const std::vector<WeightedIndex>& weighted, std::uint64_t seed);
std::size_t chooseWeightedIndexMutable(const std::vector<WeightedIndex>& weighted);

} // namespace protoseq
