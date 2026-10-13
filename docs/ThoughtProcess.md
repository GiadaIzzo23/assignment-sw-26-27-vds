# Thought process

## The problem

The dashboard must show the right information, at the right time, with the right
importance. A single "page with all the numbers" fails the driver (too much) and
the workshop (too little detail), so the design starts from *who is looking*.

## Three audiences, three hierarchies

- **Debugging** (engineer, car stationary): density wins. Every signal is on
  screen; warnings are colour on the number itself plus a chip, because the
  engineer is already reading the numbers.
- **Manual driving** (driver, car moving): attention is the scarce resource.
  Only speed, SoC, key settings and pedals are shown. Everything else is hidden
  until it matters, and then it interrupts: banner, and a flashing banner and
  frame when critical.
- **Driverless** (before/after an autonomous run): nobody is driving, so the
  question is "is the system ready?". AS state, mission, EBS, tank and brake-line
  pressure, and a readiness checklist replace speed and pedals.

## Separation of concerns

- `VehicleState` is plain data and is the only thing the UI reads.
- `IVehicleSource` is the seam between "the car" and "the screen". `Simulation`
  implements it today; a CAN/telemetry decoder can implement the same interface
  later without any change to the UI.
- `core` never links SDL. This is enforced by the build (`vds_core` has no SDL
  dependency) and lets the whole logic be tested without a display.
- `Simulation` has no wall clock: the caller passes `dt`. Tests are deterministic
  and a log replayer could drive it too.

## Warnings: detection vs presentation

Two separate questions with separate code:

1. *Is something wrong, and how badly?* `core/Warnings`: pure threshold
   evaluation with three severities (caution, warning, critical).
2. *How do I show it in this mode?* `ui/WarningPolicy`: a pure function
   `(mode, signal, severity) -> presentation`. The same inverter over-temperature
   is a tinted number in Debugging, a flashing banner in Manual and a chip in
   Driverless.

Keeping presentation out of both the detection code and the drawing code makes
the rule a single, unit-tested table instead of `if`s scattered through the
renderer.

### Hysteresis

A value jittering around a threshold would make an alert flicker on and off,
which is worse than no alert. De-escalation requires the value to recover by 
a margin (`hysteresis`), kept per signal.

## Configuration

All thresholds and simulation constants live in `Config` with built-in defaults,
overridable from `config/vds.cfg` without recompiling. Invalid or missing entries
fall back to the defaults and are reported on stderr instead of crashing.

The pages contain no thresholds or scales: they ask the warning engine or read
`Config`. Pixel layout and colours are the only numbers left in the renderer.

## Simulation scenarios

Faults are injected as a bit mask (`F1`..`F7`), so scenarios can be combined and
cleared independently. Each one drives a physical quantity (heating rate,
SoC drain, tank leak rate), and the warning engine reacts to the *measured
value*, not to the fault flag. This way the dashboard is tested on the same path
a real car would exercise.

## Testing

Automated tests cover state transitions, threshold evaluation, hysteresis,
warning detection, the per-mode presentation policy, configuration parsing and
determinism of the simulation. Graphics are not tested automatically; they are
a thin layer over already-tested logic.

## Known limitations / next steps

- The layout is fixed on a 1280x720 logical canvas scaled to the window; a real
  display would need its own layout pass.
- Pixel positions and colours are constants in `Pages.cpp`: they are presentation,
  not behaviour. The bar scales (steering, pressure) come from `Config`.
- No log replay source yet, though `IVehicleSource` allows one.
