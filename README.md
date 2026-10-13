# Vehicle Dashboard Simulator (VDS)

Desktop simulator of the Formula Student dashboard (Race UP, Task 3), written in
C++17 with SDL2. It has three dashboards with different information hierarchy,
an interactive internal vehicle simulation, and warnings whose presentation
depends on the active mode.

## Build

Tested on Ubuntu 24.04 (also under WSL2 with WSLg).

```bash
sudo apt install build-essential cmake pkg-config libsdl2-dev libsdl2-ttf-dev
git clone https://github.com/il-tuo-utente/assignment-sw-26-27-vds.git && cd assignment-sw-26-27-vds
cmake -S . -B build
cmake --build build -j
./build/vds                 # optional: ./build/vds path/to/other.cfg
ctest --test-dir build --output-on-failure
```

The tests need no graphics. To build only the logic and its tests (no SDL needed):

```bash
cmake -S . -B build -DVDS_BUILD_APP=OFF
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The build copies `assets/` and `config/` next to the executable.

## Dashboard modes

| Key | Mode | Audience | Content |
|-----|------|----------|---------|
| `1` | Debugging | workshop engineers, car stationary | maximum density: vehicle, settings, accumulator, all ECUs, per-inverter data, autonomous system, injected faults |
| `2` | Manual Driving | driver on track | speed, SoC, power/regen/TV/DRS, drive state, pedals, steering. Nothing else unless something goes wrong |
| `3` | Driverless | preparing/after an autonomous run | AS state, mission, ASMS, EBS, air tank and brake line pressure, readiness checklist |

### How warnings are presented per mode

Detection is the same everywhere (`core/Warnings`); presentation is decided by
`ui/WarningPolicy.hpp`.

| Severity | Debugging | Manual | Driverless |
|----------|-----------|--------|------------|
| Caution | number tinted | small chip (thermal/SoC); secondary systems hidden | thermal/SoC hidden; AS/EBS/tank/ECU tinted |
| Warning | chip in the alert strip | banner; secondary systems hidden | AS/EBS/tank/ECU banner; thermal/SoC chip |
| Critical | chip in the alert strip | flashing banner + flashing screen frame (secondary systems: static banner) | AS/EBS/tank/ECU flashing banner; thermal/SoC banner |

## Keyboard controls

| Key | Action |
|-----|--------|
| `W` / Up | accelerate (hold) |
| `S` / Down | brake (hold) |
| `A` `D` / Left Right | steer (hold) |
| `1` `2` `3` | Debugging / Manual / Driverless |
| `T` | tractive system on/off (precharge ~2 s) |
| `Enter` | ready to drive (hold brake) |
| `Z` / `X` | power level down / up |
| `C` / `V` | regen level down / up |
| `B` | cycle torque vectoring |
| `Space` | DRS request (needs speed and little steering) |
| `M` | ASMS on/off |
| `Q` / `E` | previous / next mission |
| `G` | GO (countdown) |
| `K` | mission complete |
| `Backspace` | emergency stop |
| `F1`..`F7` | toggle fault: inverter over-temp, critical SoC, AS fault, EBS fault, tank leak, BMS fault, SMU lost |
| `F8` | clear all faults |
| `F9` | reset the simulation |
| `Esc` | quit |

Quick start: press `T`, wait for READY, hold `S` and press `Enter`, release `S`, hold `W`.

## Architecture

```text
keyboard -> Input -> Command / DriverInput -> IVehicleSource -> VehicleState -> Pages
                                                    ^                |
                                               Simulation     WarningEngine -> WarningPolicy
```

- `src/core` (library `vds_core`, never links SDL):
  - `VehicleState`: plain data, the only thing the UI reads.
  - `IVehicleSource`: the seam. `Simulation` implements it today; a CAN/telemetry
    reader can implement the same interface without touching the UI.
  - `Simulation`: deterministic, fixed integration step, driven only by `update(dt)`.
  - `Warnings`: pure threshold evaluation (rising/falling signals) with hysteresis,
    so that jittering values do not flicker the alerts.
  - `Config`: every constant and threshold, with built-in defaults.
- `src/ui` (SDL2): `Gfx` (thin SDL wrapper), `Input` (the only place that knows
  scancodes), `Pages` (the three dashboards), `WarningPolicy` (per-mode presentation,
  SDL-free and unit-tested).
- `src/app/main.cpp`: wires the pieces together and nothing else.

Design choices are in `docs/ThoughtProcess.md`.

## Configuration

All simulation constants and warning thresholds live in `config/vds.cfg`
(`section.key = number`, `#` for comments). Edit and restart, no recompilation.
Missing or invalid entries fall back to the built-in defaults and are reported on stderr.

## Tests

`ctest` runs `vds_tests`: vehicle state, the vehicle-source interface, configuration
parsing, warning detection and hysteresis, the per-mode presentation policy and the
simulation (state transitions, scenarios, determinism). None of them needs a display.
