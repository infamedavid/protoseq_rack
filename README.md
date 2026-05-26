# Protoseq Rack

Protoseq Rack is a VCV Rack plugin of sequencer modules.

## Gina’s ARP

Gina’s ARP is a voltage-controlled generative arpeggio/sequencer module based on the Gina’s ARP step engine from Protoseq.

For the musical and technical background behind Gina’s ARP, see the Gina’s ARP paper:
https://github.com/infamedavid/GinasARP

### Inputs
- CLOCK IN
- GATE IN
- V/OCT IN
- RANGE CV
- ODTS CV

### Outputs
- V/OCT OUT
- GATE OUT

### Controls
- KEY
- MODE
- RANGE
- ODTS
- SEED
- ARP LEN
- QNT/RAW
- Range mode (context menu)

### Behavior summary
- KEY and MODE define the fixed musical field.
- V/OCT IN selects the pivot.
- QNT quantizes incoming V/OCT to KEY + MODE.
- RAW preserves incoming pitch as pivot.
- Incoming V/OCT never changes KEY or MODE.
- No mute-invalid mode.
- No Chromatic mode.
- No duplicate Natural Minor / Minor alias.

### RANGE and RangeMode
- RANGE controls the pitch window.
- Default RangeMode is **UnipolarUp**, which opens the pitch window upward from the pivot.
- **Bipolar** remains available as an alternate/advanced range mode.
- RangeMode behavior is intentional.

### ODTS
- ODTS opens progressive out-of-mode high-register notes.
- ODTS does not change KEY or MODE.
- ODTS does not reshape tonal weights.
- ODTS is not harmonic correction.
- ODTS is not tritone avoidance.

### SEED
- SEED = 0: mutable/random behavior.
- SEED > 0: deterministic seed buckets (roughly 1..1000).
- SEED has no dedicated CV input.

### ARP LEN
- ARP LEN range is 2..16.
- ARP LEN controls return-to-pivot cadence.
- Gate  len is controld for the incoming clock

### Clock/Gate behavior
- GATE IN keeps the phrase alive.
- CLOCK IN triggers notes only while GATE IN is high.
- GATE OUT is 10V while CLOCK IN and GATE IN are both high, otherwise 0V.
- GATE OUT follows CLOCK pulse duration while GATE IN is high, with normalized 10V amplitude.
- V/OCT OUT holds last pitch when GATE IN is low.

## Gina’s Expander

Gina’s Expander is a 3 HP expander for Gina’s ARP and should be placed directly to the right of it.

### Inputs
- SEED
- ALEN

### Behavior summary
- Both inputs are stepped/quantized.
- SEED CV selects mutable/random behavior or deterministic seed buckets.
- ALEN CV selects return-to-pivot length from 2 to 16.
- A patched expander input overrides the matching Gina’s ARP control.
- An unpatched expander input leaves the Gina’s ARP knob active.
- No attenuators.
- No outputs.
- No standalone note/gate generation.

## Build/Test
- `make test-core`
- `make` (if Rack SDK is installed/configured)

## License
GPL-3.0-or-later.

Copyright (C) 2026 InfameDavid / David Rodriguez
