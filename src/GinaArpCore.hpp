#pragma once

#include "GinaArpScales.hpp"

#include <vector>

namespace protoseq {

enum class PivotInputMode {
    Quantized,
    Raw
};

enum class RangeMode {
    UnipolarUp,
    Bipolar
};

enum class RegisterZone {
    Zone0,
    Zone1,
    Zone2,
    Zone3,
    Zone4,
    Zone5,
    Zone6,
};

struct GinaCandidate {
    int midiNote;
    int intervalClass;
    RegisterZone zone;
    float weight;
    bool oddity;
};

struct GinaArpContext {
    int keyRootSemitone;
    Mode mode;
    int pivotMidi;
    float effectiveRange;
    float effectiveODTS;
    int arpLen;
    float seedControl;
    int noteIndex;
    RangeMode rangeMode = RangeMode::UnipolarUp;
    bool includePivot = false;

    GinaArpContext(
        int keyRootSemitone = 0,
        Mode mode = Mode::Major,
        int pivotMidi = 60,
        float effectiveRange = 0.0f,
        float effectiveODTS = 0.0f,
        int arpLen = 4,
        float seedControl = 0.0f,
        int noteIndex = 0,
        RangeMode rangeMode = RangeMode::UnipolarUp,
        bool includePivot = false
    ) : keyRootSemitone(keyRootSemitone),
        mode(mode),
        pivotMidi(pivotMidi),
        effectiveRange(effectiveRange),
        effectiveODTS(effectiveODTS),
        arpLen(arpLen),
        seedControl(seedControl),
        noteIndex(noteIndex),
        rangeMode(rangeMode),
        includePivot(includePivot) {}
};

int voltageToNearestMidi(float volts);
float midiToVoltage(int midi);
bool shouldForcePivot(int noteIndex, int arpLen);

class GinaArpCore {
public:
    int generateMidiNote(const GinaArpContext& ctx);
    std::vector<GinaCandidate> generateCandidates(const GinaArpContext& ctx);
    int resolvePivotMidi(float vOctIn, int keyRootSemitone, Mode mode, PivotInputMode inputMode = PivotInputMode::Quantized);
};

} // namespace protoseq
