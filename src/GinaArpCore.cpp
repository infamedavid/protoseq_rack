#include "GinaArpCore.hpp"

#include "GinaArpRandom.hpp"

#include <algorithm>
#include <cmath>
#include <set>

namespace protoseq {
namespace {

constexpr int kMidiMin = 0;
constexpr int kMidiMax = 127;
constexpr int kOdditiesStartMidiNote = 84;
constexpr int kOdditiesFullMidiNote = 96;
constexpr float kOddityC6MinFactor = 0.10f;
constexpr float kOddityC6MaxFactor = 0.40f;
constexpr float kOddityC7MinFactor = 0.50f;
constexpr float kOddityC7MaxFactor = 1.00f;

template <typename T>
T clampValue(T value, T minValue, T maxValue) {
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

int clampMidi(int midi) {
    return clampValue(midi, kMidiMin, kMidiMax);
}

int mod12(int value) {
    int out = value % 12;
    if (out < 0) out += 12;
    return out;
}

float clamp01(float v) {
    return clampValue(v, 0.0f, 1.0f);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

RegisterZone zoneFromMidi(int midi) {
    if (midi < 36) return RegisterZone::Zone0;
    if (midi < 48) return RegisterZone::Zone1;
    if (midi < 60) return RegisterZone::Zone2;
    if (midi < 72) return RegisterZone::Zone3;
    if (midi < 84) return RegisterZone::Zone4;
    if (midi < 96) return RegisterZone::Zone5;
    return RegisterZone::Zone6;
}

int zoneIndex(RegisterZone zone) {
    return static_cast<int>(zone);
}

bool isIntervalAllowedInZone(
    int intervalClass,
    const std::set<int>& allowedScaleIntervalsRelativeToPivot,
    RegisterZone zone) {
    if (allowedScaleIntervalsRelativeToPivot.find(intervalClass) == allowedScaleIntervalsRelativeToPivot.end()) {
        return false;
    }

    std::set<int> allowed;
    allowed.insert(0);

    const auto has = [&allowedScaleIntervalsRelativeToPivot](int interval) {
        return allowedScaleIntervalsRelativeToPivot.find(interval) != allowedScaleIntervalsRelativeToPivot.end();
    };

    const auto addIfPresent = [&](std::initializer_list<int> intervals) {
        for (int interval : intervals) {
            if (has(interval)) {
                allowed.insert(interval);
            }
        }
    };

    if (zone >= RegisterZone::Zone1) {
        if (has(7)) {
            allowed.insert(7);
        } else if (has(6)) {
            allowed.insert(6);
        }
    }

    if (zone >= RegisterZone::Zone2) {
        if (has(3)) {
            allowed.insert(3);
        } else if (has(4)) {
            allowed.insert(4);
        }
    }

    if (zone >= RegisterZone::Zone3) {
        addIfPresent({8, 9, 10, 11});
    }

    if (zone >= RegisterZone::Zone4) {
        addIfPresent({1, 2, 5, 6});
    }

    if (zone >= RegisterZone::Zone5) {
        return true;
    }

    return allowed.find(intervalClass) != allowed.end();
}


float baseTonalWeight(int intervalClass) {
    if (intervalClass == 0) return 10.0f;
    if (intervalClass == 7) return 8.0f;
    if (intervalClass == 3 || intervalClass == 4) return 6.0f;
    if (intervalClass == 8 || intervalClass == 9 || intervalClass == 10 || intervalClass == 11) return 4.0f;
    if (intervalClass == 5) return 3.0f;
    if (intervalClass == 2) return 3.0f;
    if (intervalClass == 1) return 1.0f;
    if (intervalClass == 6) return 1.0f;
    return 1.0f;
}

int nearestQuantizedMidi(int midiIn, int keyRootSemitone, Mode mode) {
    int bestMidi = clampMidi(midiIn);
    int bestDistance = 1000;
    for (int m = kMidiMin; m <= kMidiMax; ++m) {
        if (!pitchClassInMode(keyRootSemitone, mode, m)) continue;
        const int dist = std::abs(m - midiIn);
        if (dist < bestDistance || (dist == bestDistance && m < bestMidi)) {
            bestDistance = dist;
            bestMidi = m;
        }
    }
    return bestMidi;
}

} // namespace

int voltageToNearestMidi(float volts) {
    return clampMidi(static_cast<int>(std::lround(volts * 12.0f + 60.0f)));
}

float midiToVoltage(int midi) {
    return (static_cast<float>(clampMidi(midi)) - 60.0f) / 12.0f;
}

bool shouldForcePivot(int noteIndex, int arpLen) {
    if (noteIndex <= 0) return true;
    const int safeArpLen = clampValue(arpLen, 2, 128);
    return (noteIndex % safeArpLen) == 0;
}

int GinaArpCore::resolvePivotMidi(float vOctIn, int keyRootSemitone, Mode mode, PivotInputMode inputMode) {
    const int midiIn = voltageToNearestMidi(vOctIn);
    if (inputMode == PivotInputMode::Raw) {
        return midiIn;
    }
    return nearestQuantizedMidi(midiIn, keyRootSemitone, mode);
}

std::vector<GinaCandidate> GinaArpCore::generateCandidates(const GinaArpContext& ctx) {
    std::vector<GinaCandidate> candidates;

    const int pivotMidi = clampMidi(ctx.pivotMidi);
    const float effectiveRange = clamp01(ctx.effectiveRange);
    const float effectiveODTS = clamp01(ctx.effectiveODTS);
    const int rangeSemitones = static_cast<int>(std::lround(48.0f * effectiveRange));
    int windowMin = pivotMidi;
    int windowMax = pivotMidi + rangeSemitones;
    if (ctx.rangeMode == RangeMode::Bipolar) {
        windowMin = pivotMidi - rangeSemitones;
    }
    windowMin = clampMidi(windowMin);
    windowMax = clampMidi(windowMax);

    const auto allowedIntervals = scaleIntervalsRelativeToPivot(ctx.mode, ctx.keyRootSemitone, pivotMidi);

    float tonalWeightCeiling = 0.01f;

    for (int midi = windowMin; midi <= windowMax; ++midi) {
        if (!pitchClassInMode(ctx.keyRootSemitone, ctx.mode, midi)) continue;

        const int intervalClass = mod12(midi - pivotMidi);
        const RegisterZone zone = zoneFromMidi(midi);
        if (!isIntervalAllowedInZone(intervalClass, allowedIntervals, zone)) continue;

        float weight = baseTonalWeight(intervalClass);
        if (intervalClass == 1 || intervalClass == 2 || intervalClass == 5 || intervalClass == 6) {
            weight *= (1.0f + effectiveRange);
        }
        weight *= (1.0f + static_cast<float>(zoneIndex(zone)) * 0.10f);
        weight = std::max(weight, 0.01f);
        tonalWeightCeiling = std::max(tonalWeightCeiling, weight);

        candidates.push_back({midi, intervalClass, zone, weight, false});
    }

    if (effectiveODTS <= 0.0f) {
        if (candidates.empty()) {
            candidates.push_back({pivotMidi, 0, zoneFromMidi(pivotMidi), 10.0f, false});
        }
        return candidates;
    }

    const auto oddityPitchClasses = oddityPitchClassesRelativeToKey(ctx.keyRootSemitone, ctx.mode);
    if (oddityPitchClasses.empty()) {
        if (candidates.empty()) {
            candidates.push_back({pivotMidi, 0, zoneFromMidi(pivotMidi), 10.0f, false});
        }
        return candidates;
    }

    for (int midi = std::max(windowMin, kOdditiesStartMidiNote); midi <= windowMax; ++midi) {
        if (pitchClassInMode(ctx.keyRootSemitone, ctx.mode, midi)) continue;
        const int pitchClass = mod12(midi);

        auto it = std::find(oddityPitchClasses.begin(), oddityPitchClasses.end(), pitchClass);
        if (it == oddityPitchClasses.end()) continue;

        const std::size_t rankIndex = static_cast<std::size_t>(std::distance(oddityPitchClasses.begin(), it));
        const float rankProgress = oddityPitchClasses.size() > 1
            ? static_cast<float>(rankIndex) / static_cast<float>(oddityPitchClasses.size() - 1)
            : 0.0f;

        const float registerProgress = clamp01(static_cast<float>(midi - kOdditiesStartMidiNote) /
                                               static_cast<float>(kOdditiesFullMidiNote - kOdditiesStartMidiNote));
        const float factorAtC6 = lerp(kOddityC6MinFactor, kOddityC6MaxFactor, rankProgress);
        const float factorAtC7 = lerp(kOddityC7MinFactor, kOddityC7MaxFactor, rankProgress);
        const float progressiveFactor = lerp(factorAtC6, factorAtC7, registerProgress);

        float oddityWeight = tonalWeightCeiling * effectiveODTS * progressiveFactor;
        oddityWeight = clampValue(oddityWeight, 0.0f, tonalWeightCeiling);
        if (oddityWeight <= 0.0f) continue;

        candidates.push_back({midi, mod12(midi - pivotMidi), zoneFromMidi(midi), oddityWeight, true});
    }

    if (candidates.empty()) {
        candidates.push_back({pivotMidi, 0, zoneFromMidi(pivotMidi), 10.0f, false});
    }
    return candidates;
}

int GinaArpCore::generateMidiNote(const GinaArpContext& ctx) {
    const int pivotMidi = clampMidi(ctx.pivotMidi);
    const int pivotPitchClass = mod12(pivotMidi);
    if (shouldForcePivot(ctx.noteIndex, ctx.arpLen)) {
        return pivotMidi;
    }

    const auto candidates = generateCandidates(ctx);
    if (candidates.empty()) {
        return pivotMidi;
    }

    std::vector<GinaCandidate> filteredCandidates;
    const std::vector<GinaCandidate>* selectionCandidates = &candidates;
    if (!ctx.includePivot) {
        filteredCandidates.reserve(candidates.size());
        for (const auto& candidate : candidates) {
            if (mod12(candidate.midiNote) != pivotPitchClass) {
                filteredCandidates.push_back(candidate);
            }
        }
        if (!filteredCandidates.empty()) {
            selectionCandidates = &filteredCandidates;
        }
    }

    std::vector<WeightedIndex> weighted;
    weighted.reserve(selectionCandidates->size());
    for (std::size_t i = 0; i < selectionCandidates->size(); ++i) {
        weighted.push_back({i, std::max(0.01f, (*selectionCandidates)[i].weight)});
    }

    const float seedFloat = clampValue(ctx.seedControl, 0.0f, 2.0f);
    const int seedBucket = clampValue(
        static_cast<int>(std::lround(seedFloat * 1000.0f)),
        1,
        2000
    );

    std::size_t pick = 0;
    if (seedFloat <= 0.0f) {
        pick = chooseWeightedIndexMutable(weighted);
    } else {
        const int rangeBucket = clampValue(
            static_cast<int>(std::lround(clamp01(ctx.effectiveRange) * 1000.0f)),
            0,
            1000
        );
        const int safeArpLen = clampValue(ctx.arpLen, 2, 128);
        const int phrasePosition = ctx.noteIndex % safeArpLen;
        const auto stableSeed = buildDeterministicSeed(
            seedBucket,
            phrasePosition,
            pivotMidi,
            mod12(ctx.keyRootSemitone),
            static_cast<int>(ctx.mode),
            rangeBucket);
        pick = chooseWeightedIndexDeterministic(weighted, stableSeed);
    }

    if (pick >= selectionCandidates->size()) {
        return pivotMidi;
    }
    return clampMidi((*selectionCandidates)[pick].midiNote);
}

} // namespace protoseq
