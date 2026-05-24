#pragma once

#include <set>
#include <vector>

namespace protoseq {

enum class Mode {
    Major,
    Minor,
    HarmonicMinor,
    MelodicMinor,
    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,
    Locrian,
    MajorPentatonic,
    MinorPentatonic,
    Blues,
    WholeTone,
    DiminishedHalfWhole,
    DiminishedWholeHalf,
    HungarianMinor,
    PhrygianDominant,
    JapaneseIn,
    Hirojoshi,
};

const std::vector<int>& scaleIntervals(Mode mode);
bool pitchClassInMode(int keyRootSemitone, Mode mode, int midiNote);
std::vector<int> oddityPitchClassesRelativeToKey(int keyRootSemitone, Mode mode);
std::set<int> scaleIntervalsRelativeToPivot(Mode mode, int keyRootSemitone, int pivotMidi);

} // namespace protoseq
