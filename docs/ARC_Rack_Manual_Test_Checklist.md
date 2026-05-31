# ARC Rack Manual Test Checklist

Use this checklist for ARC behavior that depends on VCV Rack runtime, UI widgets, and audio-rate output timing.

## Phase 1: MAIN clock, transport, and BPM display

- [ ] MAIN OUT is silent when ARC is stopped.
- [ ] Pressing PLAY starts MAIN OUT.
- [ ] Pressing STOP stops MAIN OUT, forces MAIN OUT and ARC OUT low, and resets the next PLAY from the cycle start.
- [ ] Pressing STOP while already stopped has no audible or phase-reset effect.
- [ ] Sending a rising edge to PLAY/STOP TOGGLE IN starts MAIN OUT when ARC is stopped.
- [ ] Sending a second rising edge to PLAY/STOP TOGGLE IN stops MAIN OUT and ARC OUT.
- [ ] Falling edges on PLAY/STOP TOGGLE IN do not change playback state.
- [ ] STOP CV IN is physically present but reserved/inactive in Phase 10A.
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

## Phase 5: monophonic ARC scheduler

- [ ] ARC OUT never overlaps or stays high continuously at normal ARPC values.
- [ ] ARC OUT never overlaps or stays high continuously at high ARPC values such as 24 and 32.
- [ ] GLEN is clamped when it would collide with the next ARC event, leaving a clear low gap.
- [ ] Fractional ARPC values (`1.5`, `2.5`, `3.5`) still produce the expected event counts over 2 MAIN periods.
- [ ] BRNL skip still suppresses ARC OUT cleanly.
- [ ] STOP forces MAIN OUT and ARC OUT low.

## Phase 6: RLEN random gate shortening

- [ ] RLEN at 0.0 leaves ARC OUT gate length stable at the GLEN/scheduler result.
- [ ] Raising RLEN only shortens ARC OUT gates and never lengthens them beyond GLEN.
- [ ] RLEN never creates ARC OUT overlap or removes the scheduler safety gap.
- [ ] BRNL-skipped steps remain silent and do not emit ARC OUT.
- [ ] MAIN OUT is unaffected by RLEN at all settings.
- [ ] With SEED above 0, the same SEED, BAR, GLEN, and RLEN settings repeat the same gate-length pattern by BAR.
- [ ] With SEED at 0 and RLEN above 0, ARC OUT gate lengths are mutable/irrepeating.

## Phase 7: RATCH ratchet bursts

- [ ] BRNL-skipped ARC steps never emit ratchets.
- [ ] NRTC at 1 produces normal single ARC gates with no ratchet subdivision.
- [ ] RRTC at 0.0 produces no ratchets.
- [ ] RRTC at 1.0 ratchets every eligible non-skipped ARC step when NRTC is above 1.
- [ ] Ratchet subpulses fit inside the final effective ARC gate length and do not extend the ARC step.
- [ ] Ratchets leave low gaps where possible and avoid stuck-high/overlap at high ARPC and high NRTC.
- [ ] MAIN OUT is unaffected by NRTC/RRTC at all settings.
- [ ] With SEED above 0, the same SEED, BAR, NRTC, and RRTC settings repeat the same ratchet positions by BAR.
- [ ] With SEED at 0, RRTC above 0, and NRTC above 1, ratchet selection is mutable/irrepeating.

## Phase 8: SWNG selective swing

- [ ] SWNG at 0.0 produces no ARC timing delay even when RSWN is raised.
- [ ] RSWN at 0.0 selects no ARC steps for swing even when SWNG is raised.
- [ ] RSWN at 1.0 applies the fixed SWNG delay to every eligible ARC event.
- [ ] With SEED above 0, the same SEED, BAR, SWNG, and RSWN settings repeat the same swung step positions by BAR.
- [ ] With SEED at 0, RSWN above 0, and SWNG above 0, swung step selection is mutable/irrepeating.
- [ ] A swung final BAR step does not overlap the first step of the next BAR.
- [ ] The final step of a BAR clamps against the real swung or unswung start time of the BAR lookahead step.
- [ ] RLEN plus SWNG does not create overlaps or stuck-high ARC OUT gates.
- [ ] RATCH plus SWNG keeps ratchet subpulses inside the final effective gate duration.
- [ ] BRNL-skipped ARC steps remain silent and do not emit swung gates or ratchets.
- [ ] MAIN OUT is unaffected by SWNG/RSWN at all settings.

## Phase 9: final CV/tooltips/polish regression

- [ ] Every knob CV input replaces its corresponding knob rather than summing with it.
- [ ] MAIN CV IN clamps below 0V to 20 BPM and above 1V to 350 BPM.
- [ ] ARPC CV IN scans only snapped valid multiplier-table entries from index 0 through 20.
- [ ] BAR CV IN and NRTC CV IN snap to their documented integer ranges.
- [ ] SEED CV IN keeps 0 as mutable mode and maps values above 0 to fixed seed buckets 1..1000.
- [ ] Parameter, input, and output tooltips clearly describe each ARC control and jack.
- [ ] Bottom jack row remains PLAY/STOP TOGGLE IN, reserved STOP CV IN, PLAY/STOP GATE IN, MAIN OUT, ARC OUT.
- [ ] No PAUSE, RESET, transport output, BRNL output, EOC output, external clock sync, or extra output appears in the UI or context menu.
- [ ] Full patch behavior works with ARC MAIN OUT to Gina’s ARP GATE IN and ARC ARC OUT to Gina’s ARP CLOCK IN.
- [ ] BRNL, RLEN, NRTC/RRTC, and SWNG/RSWN can be combined without stuck-high or overlapping ARC OUT gates.
- [ ] Fixed SEED settings repeat the same rhythmic pattern by BAR, while SEED 0 remains mutable for active random processes.
- [ ] STOP stress while ratchets or swung gates are pending forces MAIN OUT and ARC OUT low.


## Phase 10A: PLAY/STOP toggle input

- [ ] With ARC stopped, a rising edge at PLAY/STOP TOGGLE IN starts playback.
- [ ] With ARC playing, a rising edge at PLAY/STOP TOGGLE IN stops playback and forces MAIN OUT and ARC OUT low.
- [ ] Falling edges at PLAY/STOP TOGGLE IN do nothing.
- [ ] STOP CV IN remains physically present but does not control transport in this phase.
- [ ] PLAY/STOP GATE IN remains unchanged: rising edge resets the internal cycle and plays; falling edge stops.
- [ ] PLAY button still starts playback.
- [ ] STOP button still stops playback and resets the internal cycle.
- [ ] No BAR OUT exists yet.
