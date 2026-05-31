# ARC Rack Manual Test Checklist

Use this checklist for ARC Phase 1 behavior that depends on VCV Rack runtime, UI widgets, and audio-rate output timing.

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
- [ ] ARC OUT remains low throughout Phase 1.
