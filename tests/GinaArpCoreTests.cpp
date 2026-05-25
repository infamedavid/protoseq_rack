#include "../src/GinaArpCore.hpp"
#include "../src/GinaArpRandom.hpp"
#include "../src/GinaArpScales.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <vector>

using namespace protoseq;

namespace {

bool approx(float a, float b, float eps = 1e-6f) { return std::fabs(a - b) <= eps; }
float clamp01(float v) { return std::max(0.0f, std::min(1.0f, v)); }
float lerp(float a, float b, float t) { return a + (b - a) * t; }

std::vector<GinaCandidate> tonalOnly(const std::vector<GinaCandidate>& in) {
    std::vector<GinaCandidate> out;
    for (const auto& c : in) if (!c.oddity) out.push_back(c);
    return out;
}

}

int main() {
    GinaArpCore core;

    // 1) Scale interval tests
    const std::map<Mode, std::vector<int>> expected = {
        {Mode::Major, {0,2,4,5,7,9,11}}, {Mode::Minor, {0,2,3,5,7,8,10}},
        {Mode::HarmonicMinor, {0,2,3,5,7,8,11}}, {Mode::MelodicMinor, {0,2,3,5,7,9,11}},
        {Mode::Dorian, {0,2,3,5,7,9,10}}, {Mode::Phrygian, {0,1,3,5,7,8,10}},
        {Mode::Lydian, {0,2,4,6,7,9,11}}, {Mode::Mixolydian, {0,2,4,5,7,9,10}},
        {Mode::Locrian, {0,1,3,5,6,8,10}}, {Mode::MajorPentatonic, {0,2,4,7,9}},
        {Mode::MinorPentatonic, {0,3,5,7,10}}, {Mode::Blues, {0,3,5,6,7,10}},
        {Mode::WholeTone, {0,2,4,6,8,10}}, {Mode::DiminishedHalfWhole, {0,1,3,4,6,7,9,10}},
        {Mode::DiminishedWholeHalf, {0,2,3,5,6,8,9,11}}, {Mode::HungarianMinor, {0,2,3,6,7,8,11}},
        {Mode::PhrygianDominant, {0,1,4,5,7,8,10}}, {Mode::JapaneseIn, {0,1,5,7,10}},
        {Mode::Hirojoshi, {0,2,3,7,8}}
    };
    for (const auto& [mode, ivals] : expected) assert(scaleIntervals(mode) == ivals);
    assert(expected.count(Mode::Minor) == 1);

    // 2) V/OCT conversions
    assert(voltageToNearestMidi(0.0f) == 60);
    assert(voltageToNearestMidi(1.0f) == 72);
    assert(voltageToNearestMidi(-1.0f) == 48);
    assert(approx(midiToVoltage(60), 0.0f));
    assert(approx(midiToVoltage(72), 1.0f));
    assert(approx(midiToVoltage(48), -1.0f));
    assert(approx(midiToVoltage(61) - midiToVoltage(60), 1.0f/12.0f));

    // 3) QNT pivot tests KEY C MODE Major
    assert(core.resolvePivotMidi(midiToVoltage(61), 0, Mode::Major, PivotInputMode::Quantized) == 60);
    assert(core.resolvePivotMidi(midiToVoltage(63), 0, Mode::Major, PivotInputMode::Quantized) == 62);
    assert(core.resolvePivotMidi(midiToVoltage(66), 0, Mode::Major, PivotInputMode::Quantized) == 65);
    assert(core.resolvePivotMidi(midiToVoltage(68), 0, Mode::Major, PivotInputMode::Quantized) == 67);
    assert(core.resolvePivotMidi(midiToVoltage(70), 0, Mode::Major, PivotInputMode::Quantized) == 69);

    // 4) RAW pivot tests
    assert(core.resolvePivotMidi(midiToVoltage(61), 0, Mode::Major, PivotInputMode::Raw) == 61);
    GinaArpContext rawCtx{0, Mode::Major, 61, 0.3f, 0.0f, 4, 0.4f, 1};
    for (const auto& c : core.generateCandidates(rawCtx)) {
        if (!c.oddity) assert(pitchClassInMode(0, Mode::Major, c.midiNote));
    }

    // 5) RANGE mapping and clamp
    GinaArpContext r0{0, Mode::Major, 60, 0.0f, 0.0f, 4, 0.2f, 1};
    auto c0 = core.generateCandidates(r0);
    for (const auto& c : tonalOnly(c0)) assert(c.midiNote >= 60 && c.midiNote <= 60);
    GinaArpContext r05{0, Mode::Major, 60, 0.5f, 0.0f, 4, 0.2f, 1};
    auto c05 = core.generateCandidates(r05);
    for (const auto& c : tonalOnly(c05)) assert(c.midiNote >= 36 && c.midiNote <= 84);
    GinaArpContext r1{0, Mode::Major, 60, 1.0f, 0.0f, 4, 0.2f, 1};
    auto c1 = core.generateCandidates(r1);
    for (const auto& c : tonalOnly(c1)) assert(c.midiNote >= 12 && c.midiNote <= 108);
    GinaArpContext rClamp{0, Mode::Major, 60, 5.0f, 0.0f, 4, 0.2f, 1};
    auto cc = core.generateCandidates(rClamp);
    for (const auto& c : tonalOnly(cc)) assert(c.midiNote >= 12 && c.midiNote <= 108);

    // 6) ARP LEN
    assert(shouldForcePivot(0, 4));
    assert(!shouldForcePivot(1, 4));
    assert(shouldForcePivot(4, 4));
    assert(shouldForcePivot(8, 4));
    assert(shouldForcePivot(12, 4));
    assert(shouldForcePivot(1, 1));

    // 7) ODTS
    GinaArpContext od0{0, Mode::Major, 84, 0.4f, 0.0f, 4, 0.2f, 1};
    auto noOdd = core.generateCandidates(od0);
    for (const auto& c : noOdd) assert(!c.oddity);

    GinaArpContext od1hi{0, Mode::Major, 96, 0.5f, 1.0f, 4, 0.2f, 1};
    auto oddHi = core.generateCandidates(od1hi);
    bool hasOdd = false;
    float tonalCeil = 0.f;
    for (const auto& c : oddHi) {
        if (!c.oddity) tonalCeil = std::max(tonalCeil, c.weight);
        if (c.oddity) {
            hasOdd = true;
            assert(c.midiNote >= 84);
            assert(c.weight <= tonalCeil + 1e-5f);
        }
    }
    assert(hasOdd);

    GinaArpContext od1low{0, Mode::Major, 72, 0.2f, 1.0f, 4, 0.2f, 1};
    auto oddLow = core.generateCandidates(od1low);
    for (const auto& c : oddLow) assert(!c.oddity);

    auto findOddWeight = [&](int midi)->float { for (auto& c: oddHi) if (c.oddity && c.midiNote==midi) return c.weight; return -1.f; };
    assert(findOddWeight(85) > 0.f && findOddWeight(97) > findOddWeight(85));
    assert(findOddWeight(94) > findOddWeight(85));
    assert(findOddWeight(106) > findOddWeight(94));

    // Exact ODTS progressive-factor formula checks (C major oddity rank: C#, D#, F#, G#, A#)
    const float cSharp6 = findOddWeight(85);   // C#6, rank 0
    const float aSharp6 = findOddWeight(94);   // A#6, rank 4
    const float cSharp7 = findOddWeight(97);   // C#7, rank 0
    const float aSharp7 = findOddWeight(106);  // A#7, rank 4
    assert(cSharp6 > 0.f && aSharp6 > 0.f && cSharp7 > 0.f && aSharp7 > 0.f);

    const auto expectedOddWeight = [&](int midi, int oddityIndex, int oddityCount)->float {
        const float registerProgress = clamp01((static_cast<float>(midi) - 84.0f) / 12.0f);
        const float rankProgress = oddityCount > 1 ? static_cast<float>(oddityIndex) / static_cast<float>(oddityCount - 1) : 0.0f;
        const float factorAtC6 = lerp(0.10f, 0.40f, rankProgress);
        const float factorAtC7 = lerp(0.50f, 1.00f, rankProgress);
        const float progressiveFactor = lerp(factorAtC6, factorAtC7, registerProgress);
        return std::min(tonalCeil, tonalCeil * progressiveFactor); // ODTS=1.0 here
    };
    assert(approx(cSharp6, expectedOddWeight(85, 0, 5), 1e-4f));
    assert(approx(aSharp6, expectedOddWeight(94, 4, 5), 1e-4f));
    assert(approx(cSharp7, expectedOddWeight(97, 0, 5), 1e-4f));
    assert(approx(aSharp7, expectedOddWeight(106, 4, 5), 1e-4f));

    // ODTS must not change tonal weights
    GinaArpContext tonalA{0, Mode::Major, 84, 0.4f, 0.0f, 4, 0.2f, 1};
    GinaArpContext tonalB{0, Mode::Major, 84, 0.4f, 1.0f, 4, 0.2f, 1};
    auto ta = tonalOnly(core.generateCandidates(tonalA));
    auto tb = tonalOnly(core.generateCandidates(tonalB));
    assert(ta.size() == tb.size());
    for (size_t i=0;i<ta.size();++i) { assert(ta[i].midiNote == tb[i].midiNote); assert(approx(ta[i].weight, tb[i].weight)); }

    // 8) Pentatonic ODTS
    GinaArpContext pent{0, Mode::MajorPentatonic, 96, 0.5f, 1.0f, 4, 0.2f, 1};
    auto pentC = core.generateCandidates(pent);
    std::set<int> oddPC;
    for (auto& c : pentC) if (c.oddity) oddPC.insert((c.midiNote%12+12)%12);
    for (int pc : std::vector<int>{1,3,5,6,8,10,11}) assert(oddPC.count(pc) > 0);

    // 9) Locrian strictness
    GinaArpContext loc{0, Mode::Locrian, 60, 0.8f, 0.0f, 4, 0.2f, 1};
    auto locC = core.generateCandidates(loc);
    bool hasPerfect5 = false, hasFlat5 = false;
    for (auto& c : locC) if (!c.oddity) { if (c.intervalClass==7) hasPerfect5=true; if (c.intervalClass==6) hasFlat5=true; }
    assert(!hasPerfect5);
    assert(hasFlat5);

    // 10) Seed tests
    // Seed mapping: <=0.0 = mutable, >0.0 mapped to deterministic buckets 1..1000.
    const auto toSeedBucket = [](float seedControl)->int {
        return std::clamp(static_cast<int>(std::lround(seedControl * 1000.0f)), 1, 1000);
    };
    assert(toSeedBucket(-4.2f) == 1);
    assert(toSeedBucket(0.001f) == 1);
    assert(toSeedBucket(1.0f) == 1000);

    GinaArpContext sd{0, Mode::Major, 72, 0.4f, 0.0f, 4, 1.0f, 3};

    // With fixed seed and ARP LEN=4, phrase positions should repeat every 4 notes.
    GinaArpContext len4 = sd;
    len4.seedControl = 0.456f;
    len4.arpLen = 4;
    len4.noteIndex = 1; const int len4n1 = core.generateMidiNote(len4);
    len4.noteIndex = 5; const int len4n5 = core.generateMidiNote(len4);
    assert(len4n1 == len4n5);
    len4.noteIndex = 2; const int len4n2 = core.generateMidiNote(len4);
    len4.noteIndex = 6; const int len4n6 = core.generateMidiNote(len4);
    assert(len4n2 == len4n6);
    len4.noteIndex = 3; const int len4n3 = core.generateMidiNote(len4);
    len4.noteIndex = 7; const int len4n7 = core.generateMidiNote(len4);
    assert(len4n3 == len4n7);

    // With fixed seed and ARP LEN=5, phrase positions should repeat every 5 notes.
    GinaArpContext len5 = sd;
    len5.seedControl = 0.456f;
    len5.arpLen = 5;
    len5.noteIndex = 1; const int len5n1 = core.generateMidiNote(len5);
    len5.noteIndex = 6; const int len5n6 = core.generateMidiNote(len5);
    assert(len5n1 == len5n6);
    len5.noteIndex = 2; const int len5n2 = core.generateMidiNote(len5);
    len5.noteIndex = 7; const int len5n7 = core.generateMidiNote(len5);
    assert(len5n2 == len5n7);

    // Different fixed seeds can produce different phrase identities.
    bool differentPhraseIdentity = false;
    for (int idx = 1; idx < 16 && !differentPhraseIdentity; ++idx) {
        GinaArpContext a = sd;
        GinaArpContext b = sd;
        a.noteIndex = idx;
        b.noteIndex = idx;
        a.seedControl = 0.111f;
        b.seedControl = 0.777f;
        differentPhraseIdentity = (core.generateMidiNote(a) != core.generateMidiNote(b));
    }
    assert(differentPhraseIdentity);

    // Seed identity includes fixedSeed bucket, phrase position, pivot, key, mode, and range bucket.
    const int seedBucket = toSeedBucket(0.5f);
    const int safeArpLen = std::max(1, sd.arpLen);
    const int phrasePosition = sd.noteIndex % safeArpLen;
    const int rangeBucket = std::clamp(static_cast<int>(std::lround(clamp01(sd.effectiveRange) * 1000.0f)), 0, 1000);
    const std::uint64_t stableA = buildDeterministicSeed(seedBucket, phrasePosition, sd.pivotMidi, sd.keyRootSemitone, static_cast<int>(sd.mode), rangeBucket);
    const std::uint64_t stableB = buildDeterministicSeed(seedBucket, phrasePosition, sd.pivotMidi, sd.keyRootSemitone, static_cast<int>(sd.mode), rangeBucket);
    assert(stableA == stableB);

    // Mutable mode (seedControl<=0) should not collapse to deterministic repeats.
    GinaArpContext mut{0, Mode::Major, 72, 1.0f, 0.0f, 4, 0.0f, 9};
    const int mutableTrials = 32;
    int mutableFirst = core.generateMidiNote(mut);
    bool mutableVaries = false;
    for (int i = 0; i < mutableTrials; ++i) {
        if (core.generateMidiNote(mut) != mutableFirst) {
            mutableVaries = true;
            break;
        }
    }
    assert(mutableVaries);

    // 11) fallback
    GinaArpContext fb{0, Mode::Major, 1, 0.0f, 0.0f, 4, 0.2f, 1};
    auto fbc = core.generateCandidates(fb);
    assert(!fbc.empty());

    std::cout << "All GinaArp core tests passed\n";
    return 0;
}
