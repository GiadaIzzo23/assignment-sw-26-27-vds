// The seam between "the car" and "the screen".
//
// The UI only ever talks to this interface. Today it is implemented by
// Simulation; tomorrow a CAN/telemetry reader can implement the same four
// methods (apply() and setDriverInput() simply become no-ops for a real car)
// and the renderer does not change at all.
#pragma once

#include "core/VehicleState.hpp"

namespace vds {

enum class CommandType {
    TsToggle,
    ReadyToDrive,
    PowerUp,
    PowerDown,
    RegenUp,
    RegenDown,
    TvCycle,
    DrsToggle,
    MissionNext,
    MissionPrev,
    AsmsToggle,
    Go,
    EmergencyStop,
    MissionComplete,
    ToggleFault,   // uses Command::fault
    ClearFaults,
    Reset,
};

struct Command {
    CommandType type;
    Fault fault = Fault::None;
};

// What the driver is physically doing right now (held keys, not events).
struct DriverInput {
    bool accelerate = false;
    bool brake = false;
    int steer = 0;  // -1 left, 0 centre, +1 right
};

class IVehicleSource {
public:
    virtual ~IVehicleSource() = default;
    virtual void update(double dt_seconds) = 0;
    virtual const VehicleState& state() const = 0;
    virtual void apply(const Command& c) = 0;
    virtual void setDriverInput(const DriverInput& in) = 0;
};

}  // namespace vds
