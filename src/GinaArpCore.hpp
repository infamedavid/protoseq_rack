#pragma once

#include "GinaArpScales.hpp"

#include <vector>

namespace protoseq {

enum class PivotInputMode {
    Quantized,
    Raw
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
