#include "GinaArpRandom.hpp"

#include <algorithm>
#include <random>

namespace protoseq {
namespace {

std::size_t chooseWeightedIndexWithEngine(const std::vector<WeightedIndex>& weighted, std::mt19937_64& engine) {
    if (weighted.empty()) {
        return 0;
    }

    float total = 0.0f;
    for (const auto& item : weighted) {
        total += std::max(item.weight, 0.0f);
    }
    if (total <= 0.0f) {
        return weighted.front().index;
    }

    std::uniform_real_distribution<float> dist(0.0f, total);
    const float pick = dist(engine);

    float cumulative = 0.0f;
    for (const auto& item : weighted) {
        cumulative += std::max(item.weight, 0.0f);
        if (pick <= cumulative) {
            return item.index;
        }
    }
    return weighted.back().index;
}

} // namespace

std::uint64_t mixSeed(std::uint64_t state, std::uint64_t value) {
    state ^= value + 0x9e3779b97f4a7c15ULL + (state << 6U) + (state >> 2U);
    return state;
}

std::uint64_t buildDeterministicSeed(
    int seedBucket,
    int noteIndex,
    int pivotMidi,
    int keyRootSemitone,
    int mode,
    int rangeBucket) {
    std::uint64_t state = 0xcbf29ce484222325ULL;
    state = mixSeed(state, static_cast<std::uint64_t>(seedBucket));
    state = mixSeed(state, static_cast<std::uint64_t>(noteIndex));
    state = mixSeed(state, static_cast<std::uint64_t>(pivotMidi));
    state = mixSeed(state, static_cast<std::uint64_t>(keyRootSemitone));
    state = mixSeed(state, static_cast<std::uint64_t>(mode));
    state = mixSeed(state, static_cast<std::uint64_t>(rangeBucket));
    return state;
}

std::size_t chooseWeightedIndexDeterministic(const std::vector<WeightedIndex>& weighted, std::uint64_t seed) {
    std::mt19937_64 engine(seed);
    return chooseWeightedIndexWithEngine(weighted, engine);
}

std::size_t chooseWeightedIndexMutable(const std::vector<WeightedIndex>& weighted) {
    static thread_local std::mt19937_64 engine(std::random_device{}());
    return chooseWeightedIndexWithEngine(weighted, engine);
}

} // namespace protoseq
