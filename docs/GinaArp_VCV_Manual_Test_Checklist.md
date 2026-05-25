# Gina’s ARP — Local VCV Rack Manual Test Checklist

> This checklist is for local VCV Rack validation.

## Patch setup

- Clock source -> **Gina CLOCK IN**
- Gate source -> **Gina GATE IN**
- Pitch CV source -> **Gina V/OCT IN**
- **Gina V/OCT OUT** -> oscillator V/OCT
- **Gina GATE OUT** -> envelope gate
- Envelope -> VCA CV
- Oscillator -> VCA -> audio out

---

## A) Basic gate/clock behavior

1. Keep CLOCK running, set GATE low.
   - Expect: no note generation.
   - Expect: GATE OUT stays low.
2. Raise GATE high with CLOCK still running.
   - Expect: each CLOCK rising edge generates one note.
   - Expect: GATE OUT follows CLOCK pulse duration.
   - Expect: GATE OUT amplitude is normalized to 10V while high.
   - Expect: V/OCT OUT updates on CLOCK rising edges.

## B) Gate length behavior

1. Use a clock with pulse-width control.
2. Compare short vs long clock pulses.
   - Expect: short CLOCK pulse -> short Gina GATE OUT.
   - Expect: long CLOCK pulse -> long Gina GATE OUT.
   - Expect: ARP LEN does not change gate length.
   - Expect: no internal gate-length control on Gina.

## C) QNT mode

1. Set KEY=C, MODE=Major, pivot mode=QNT.
2. Feed C# into V/OCT IN.
   - Expect: pivot resolves to C.
   - Expect: KEY remains C.
   - Expect: MODE remains Major.

## D) RAW mode

1. Set KEY=C, MODE=Major, pivot mode=RAW.
2. Feed C# into V/OCT IN.
   - Expect: pivot remains C#.
   - Expect: KEY remains C.
   - Expect: MODE remains Major.
   - Expect: forced pivot events can output C#.
   - Expect: non-forced tonal candidates still obey C Major.

## E) RANGE and RangeMode

1. Confirm default RangeMode is UnipolarUp.
   - Expect: pitch window opens upward from the pivot.
2. Set RangeMode to Bipolar from context menu.
   - Expect: pitch window opens below and above the pivot.
3. Sweep RANGE from low to high.
   - Expect: low range = tight window.
   - Expect: high range = wider window.
4. Design intent check.
   - Expect: RangeMode is intentional and not a bug.

## F) ARP LEN

1. Set ARP LEN=2.
   - Expect: pivot at noteIndex 0, 2, 4, 6...
2. Set ARP LEN=4.
   - Expect: pivot at noteIndex 0, 4, 8...
3. Set ARP LEN=16.
   - Expect: pivot at noteIndex 0, 16, 32...
4. Cross-check gate behavior.
   - Expect: ARP LEN controls return-to-pivot cadence only.
   - Expect: ARP LEN does not control gate length.

## G) ODTS

1. KEY=C, MODE=Major, ODTS=0, high register source.
   - Expect: only C D E F G A B tonal classes.
2. KEY=C, MODE=Major, ODTS=1, high register source.
   - Expect: C# D# F# G# A# can appear as oddities.
3. ODTS=1 in low register.
   - Expect: no oddities below C6.

## H) ODTS curve

- Expect: C#7 weight > C#6 weight.
- Expect: A#6 weight > C#6 weight.
- Expect: A#7 weight > A#6 weight.
- Expect: oddities should not exceed tonal ceiling dominance.

## I) Pentatonic ODTS

1. KEY=C, MODE=MajorPentatonic, ODTS high.
   - Internal mode notes: C D E G A.
   - Oddities may include: C#, D#, F, F#, G#, A#, B.
   - This is expected/correct.

## J) SEED

1. SEED=0.
   - Expect: mutable/random behavior.
2. SEED>0 fixed.
   - Expect: same patch/input/noteIndex repeats deterministically.

## K) Musical acceptance (subjective local pass)

- Should feel like Gina’s ARP.
- Low register should feel more restricted.
- High register should open up.
- ODTS should read as high-register chromatic ornament, not flat chromatic chaos.
