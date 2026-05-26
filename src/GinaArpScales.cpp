#include "GinaArpScales.hpp"

#include <array>
#include <set>
#include <cassert>

namespace protoseq {
namespace {

constexpr int kPitchClassCount = 12;

int mod12(int value) {
    int result = value % kPitchClassCount;
    if (result < 0) {
        result += kPitchClassCount;
    }
    return result;
}

} // namespace

const std::vector<int>& scaleIntervals(Mode mode) {
    static const std::vector<int> major{0, 2, 4, 5, 7, 9, 11};
    static const std::vector<int> minor{0, 2, 3, 5, 7, 8, 10};
    static const std::vector<int> harmonicMinor{0, 2, 3, 5, 7, 8, 11};
    static const std::vector<int> melodicMinor{0, 2, 3, 5, 7, 9, 11};
    static const std::vector<int> dorian{0, 2, 3, 5, 7, 9, 10};
    static const std::vector<int> phrygian{0, 1, 3, 5, 7, 8, 10};
    static const std::vector<int> lydian{0, 2, 4, 6, 7, 9, 11};
    static const std::vector<int> mixolydian{0, 2, 4, 5, 7, 9, 10};
    static const std::vector<int> locrian{0, 1, 3, 5, 6, 8, 10};
    static const std::vector<int> majorPentatonic{0, 2, 4, 7, 9};
    static const std::vector<int> minorPentatonic{0, 3, 5, 7, 10};
    static const std::vector<int> blues{0, 3, 5, 6, 7, 10};
    static const std::vector<int> wholeTone{0, 2, 4, 6, 8, 10};
    static const std::vector<int> diminishedHalfWhole{0, 1, 3, 4, 6, 7, 9, 10};
    static const std::vector<int> diminishedWholeHalf{0, 2, 3, 5, 6, 8, 9, 11};
    static const std::vector<int> hungarianMinor{0, 2, 3, 6, 7, 8, 11};
    static const std::vector<int> phrygianDominant{0, 1, 4, 5, 7, 8, 10};
    static const std::vector<int> japaneseIn{0, 1, 5, 7, 10};
    static const std::vector<int> hirojoshi{0, 2, 3, 7, 8};

    switch (mode) {
        case Mode::Major: return major;
        case Mode::Minor: return minor;
        case Mode::HarmonicMinor: return harmonicMinor;
        case Mode::MelodicMinor: return melodicMinor;
        case Mode::Dorian: return dorian;
        case Mode::Phrygian: return phrygian;
        case Mode::Lydian: return lydian;
        case Mode::Mixolydian: return mixolydian;
        case Mode::Locrian: return locrian;
        case Mode::MajorPentatonic: return majorPentatonic;
        case Mode::MinorPentatonic: return minorPentatonic;
        case Mode::Blues: return blues;
        case Mode::WholeTone: return wholeTone;
        case Mode::DiminishedHalfWhole: return diminishedHalfWhole;
        case Mode::DiminishedWholeHalf: return diminishedWholeHalf;
        case Mode::HungarianMinor: return hungarianMinor;
        case Mode::PhrygianDominant: return phrygianDominant;
        case Mode::JapaneseIn: return japaneseIn;
        case Mode::Hirojoshi: return hirojoshi;
    }

    assert(false && "Unknown mode");
    return major;
}

bool pitchClassInMode(int keyRootSemitone, Mode mode, int midiNote) {
    const int key = mod12(keyRootSemitone);
    const int pitchClass = mod12(midiNote);
    const int relative = mod12(pitchClass - key);
    const auto& intervals = scaleIntervals(mode);
    for (int interval : intervals) {
        if (interval == relative) {
            return true;
        }
    }
    return false;
}

std::vector<int> oddityPitchClassesRelativeToKey(int keyRootSemitone, Mode mode) {
    const int key = mod12(keyRootSemitone);
    std::array<bool, kPitchClassCount> inMode{};
    for (int interval : scaleIntervals(mode)) {
        inMode[mod12(key + interval)] = true;
    }

    std::vector<int> oddities;
    oddities.reserve(kPitchClassCount);
    for (int rel = 0; rel < kPitchClassCount; ++rel) {
        const int pitchClass = mod12(key + rel);
        if (!inMode[pitchClass]) {
            oddities.push_back(pitchClass);
        }
    }
    return oddities;
}

std::set<int> scaleIntervalsRelativeToPivot(Mode mode, int keyRootSemitone, int pivotMidi) {
    const int key = mod12(keyRootSemitone);
    const int pivotClass = mod12(pivotMidi);

    std::set<int> relativeIntervals;
    for (int intervalFromKey : scaleIntervals(mode)) {
        const int scalePitchClass = mod12(key + intervalFromKey);
        relativeIntervals.insert(mod12(scalePitchClass - pivotClass));
    }
    return relativeIntervals;
}

} // namespace protoseq
