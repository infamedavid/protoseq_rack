# Gina’s ARP — Local VCV Rack Manual Test Checklist (Phase 10)

> This checklist is for **local** VCV Rack testing by David.
> It is not executed in Codex Cloud.

## Patch setup

- Clocked (or any clock module) -> **Gina CLOCK IN**
- Gate sequencer / keyboard gate / manual gate -> **Gina GATE IN**
- Keyboard CV / sequencer CV / S&H CV -> **Gina V/OCT IN**
- **Gina V/OCT OUT** -> oscillator V/OCT
- **Gina GATE OUT** -> envelope gate
- Envelope -> VCA CV
- Oscillator -> VCA -> audio out

---

## A) Basic gate/clock

1. Keep CLOCK running, set GATE low.
   - Expect: no note generation.
   - Expect: GATE OUT stays low.
2. Raise GATE high with CLOCK still running.
   - Expect: each CLOCK rising edge generates one note.
   - Expect: GATE OUT follows CLOCK pulse duration.
   - Expect: V/OCT OUT updates on CLOCK rising edges.

## B) Gate length behavior

1. Use Clocked pulse-width control.
2. Compare short vs long pulses.
   - Expect: short CLOCK pulse -> short Gina GATE OUT.
   - Expect: long CLOCK pulse -> long Gina GATE OUT.
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

## E) RANGE

1. RANGE=0.
   - Expect: output stays at/near pivot (pivot-locked behavior).
2. RANGE=medium.
   - Expect: moderate pitch window.
3. RANGE=high.
   - Expect: wider pitch window, including higher candidates.

## F) ARP LEN

1. ARP LEN=1.
   - Expect: every clock event forces pivot.
2. ARP LEN=4.
   - Expect: noteIndex 0,4,8,... force pivot.
   - Expect: ARP LEN does not control gate length.

## G) ODTS

1. KEY=C, MODE=Major, ODTS=0, high register source.
   - Expect: only C D E F G A B tonal classes.
2. KEY=C, MODE=Major, ODTS=1, high register source.
   - Expect: C# D# F# G# A# can appear as oddities.
3. ODTS=1 in low register.
   - Expect: no oddities below C6.

## H) ODTS curve

Using listening and/or debug logging:

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
   - Expect: mutable behavior (changes across clock events).
2. SEED>0 fixed.
   - Expect: same patch/input/noteIndex repeats deterministically.
3. Modulate SEED CV.
   - Expect: identity changes by seed bucket, not per-sample chaos.

## K) Musical acceptance (local-only subjective pass)

- Should feel like Gina’s ARP, not generic random arp.
- Low register should feel more restricted.
- High register should open up.
- ODTS should read as high-register chromatic ornament, not flat chromatic chaos.
