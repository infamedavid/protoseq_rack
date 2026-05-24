# Protoseq Rack

Protoseq Rack is the **VCV Rack plugin/brand**.
Gina’s ARP is one module inside Protoseq Rack, and future Protoseq modules may be added later.

## Gina’s ARP (module)

Gina’s ARP is based on the Gina’s ARP step engine from Protoseq.

- Inputs: **CLOCK IN**, **GATE IN**, **V/OCT IN** (+ RANGE CV, ODTS CV, SEED CV).
- Outputs: **V/OCT OUT**, **GATE OUT**.
- **V/OCT IN** selects pivot.
- **KEY** and **MODE** remain fixed selector states (they are not rewritten by input pitch).
- **RANGE** controls pitch reach around pivot.
- **ODTS** opens progressive high-register chromatic oddities (without changing KEY or MODE).
- **SEED** controls mutable/fixed generative identity:
  - **SEED = 0** => mutable behavior
  - **SEED > 0** => deterministic seed buckets (1..1000)
- **ARP LEN** controls return-to-pivot cadence (not gate length).
- **GATE OUT** follows external CLOCK pulse duration while GATE IN is high.
- There is **no internal gate length** control.
- V/OCT pivot mode switch:
  - **QNT** (default): quantize incoming pivot to KEY+MODE
  - **RAW**: use incoming pitch directly as pivot
- No Chromatic mode.
- No NaturalMinor duplicate.

For local manual validation in VCV Rack, see:

- `docs/GinaArp_VCV_Manual_Test_Checklist.md`
