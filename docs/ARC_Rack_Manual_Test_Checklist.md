# ARC Rack Manual Test Checklist

Use this checklist for ARC behavior that depends on VCV Rack runtime, UI widgets, and audio-rate output timing.

## Phase 1: MAIN clock, transport, and BPM display

- [ ] MAIN OUT is silent when ARC is stopped.
- [ ] Pressing PLAY starts MAIN OUT.
- [ ] Pressing STOP stops MAIN OUT, forces MAIN OUT and ARC OUT low, and resets the next PLAY from the cycle start.
- [ ] Pressing STOP while already stopped has no audible or phase-reset effect.
- [ ] Sending a rising edge to PLAY CV IN starts MAIN OUT.
- [ ] Sending a rising edge to STOP CV IN stops MAIN OUT, forces MAIN OUT and ARC OUT low, and resets the next PLAY from the cycle start.
- [ ] PLAY/STOP GATE IN rising edge starts playback from the beginning of the MAIN cycle.
- [ ] PLAY/STOP GATE IN falling edge stops playback and forces MAIN OUT and ARC OUT low.
- [ ] MAIN CV IN, when patched, replaces the MAIN knob for BPM control.
- [ ] MAIN CV IN at 0.0V displays `020` and runs MAIN OUT at 20 BPM.
- [ ] MAIN CV IN at 1.0V displays `350` and runs MAIN OUT at 350 BPM.
- [ ] MAIN CV IN below 0.0V clamps to `020`.
- [ ] MAIN CV IN above 1.0V clamps to `350`.
- [ ] With MAIN CV IN unpatched, the BPM display follows the MAIN knob rounded to a three-digit integer.
- [ ] PW CV IN, when patched, replaces the PW knob for MAIN OUT pulse width control.
- [ ] MAIN OUT is 10V while high and 0V while low.
- [ ] PW leaves a visible low gap in MAIN OUT even when the PW knob or PW CV IN is at maximum.

## Phase 2: ARC OUT multiplier and gate length

- [ ] ARPC index 0 (`1`) produces one ARC pulse per MAIN period.
- [ ] ARPC index 1 (`1.5`) produces 3 ARC pulses over 2 MAIN periods.
- [ ] ARPC index 3 (`2.5`) produces 5 ARC pulses over 2 MAIN periods.
- [ ] ARPC index 5 (`3.5`) produces 7 ARC pulses over 2 MAIN periods.
- [ ] ARPC index 20 (`32`) is reachable and produces 32 ARC pulses per MAIN period.
- [ ] ARC phase does not reset on every MAIN pulse.
- [ ] PLAY/STOP GATE IN rising resets ARC phase and starts from the beginning of the ARC cycle.
- [ ] STOP forces ARC OUT low.
- [ ] GLEN controls ARC OUT high duration without producing overlapping or stuck-high ARC gates.
- [ ] GLEN CV IN, when patched, replaces the GLEN knob.
- [ ] ARPC CV IN, when patched, replaces the ARPC knob and scans the snapped index range 0..20.
- [ ] BAR and BAR CV IN advance as an ARC-event cycle foundation without audible random/seed behavior in this phase.

## Phase 3: SEED and BAR deterministic random foundation

- [ ] SEED knob and SEED CV IN produce no audible random behavior yet.
- [ ] SEED CV IN, when patched, replaces the SEED knob for the global ARC seed bucket.
- [ ] SEED at 0 remains the mutable/irrepeating mode foundation for future random processes.
- [ ] SEED above 0 selects fixed seed buckets 1..1000 for future random processes.
- [ ] BAR remains measured in ARC events and wraps the internal bar step by the effective BAR length.

## Phase 4: BRNL skip

- [ ] MAIN OUT is unaffected by BRNL at all BRNL settings.
- [ ] ARC OUT is skipped according to BRNL skip probability.
- [ ] BRNL at 0.0 passes all ARC gates.
- [ ] BRNL at 1.0 silences ARC OUT while MAIN OUT keeps running.
- [ ] With SEED above 0, the same SEED, BAR, and BRNL settings repeat the same ARC OUT skip positions by BAR.
- [ ] With SEED at 0 and BRNL between 0.0 and 1.0, ARC OUT skip positions are mutable/irrepeating.
