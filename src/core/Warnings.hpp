// Warning detection: pure functions of VehicleState + Config thresholds.
// Independent of how the state was produced (simulation or real car) and of how
// it will be drawn (see WarningPolicy.hpp for the per-mode presentation).
#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/Config.hpp"
#include "core/VehicleState.hpp"

namespace vds {

enum class Severity { None = 0, Caution = 1, Warning = 2, Critical = 3 };

enum class WarnId {
    InverterTemp,   // index = inverter
    MotorTemp,      // index = inverter
    PackTemp,
    LowSoc,
    LowTankPressure,
    EcuStatus,      // index = Ecu
    EbsStatus,      // EBS unavailable / activated
    AsEmergency,
};

struct Warning {
    WarnId id;
    int index;          // which inverter / ECU (0 when not applicable)
    Severity severity;
    double value;       // the offending measurement (0 for discrete events)
};

const char* toString(Severity);
std::string describe(const Warning&);  // e.g. "INV FL TEMP 87 C"

// Stateless threshold evaluation (no hysteresis): the severity a value maps to.
Severity classifyRising(double value, const ThresholdSet& t);
Severity classifyFalling(double value, const ThresholdSet& t);

// Stateful engine: remembers the previous severity of every signal so that
// de-escalation needs the value to recover by `hysteresis` (prevents a number
// that jitters around a threshold from flickering the alert on and off).
class WarningEngine {
public:
    explicit WarningEngine(const Config& cfg) : cfg_(cfg) {}

    // Returns active warnings sorted by severity (highest first), then by id.
    const std::vector<Warning>& evaluate(const VehicleState& s);
    const std::vector<Warning>& active() const { return active_; }

    Severity worst() const;
    // Severity currently latched for one signal (None if not active).
    Severity severityOf(WarnId id, int index = 0) const;
    void reset();

private:
    Severity step(WarnId id, int index, Severity raw, double value, bool rising,
                  const ThresholdSet* t);
    const Config& cfg_;
    std::map<std::pair<int, int>, Severity> latched_;
    std::vector<Warning> active_;
};

}  // namespace vds
