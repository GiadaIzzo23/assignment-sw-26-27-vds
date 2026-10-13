// Internal vehicle simulation: the only IVehicleSource that exists today.
// Pure logic: no SDL, no wall clock, no I/O. The caller drives time through
// update(dt), so tests (and a future log replayer) are fully deterministic.
#pragma once

#include "core/Config.hpp"
#include "core/IVehicleSource.hpp"

namespace vds {

class Simulation : public IVehicleSource {
public:
    explicit Simulation(const Config& cfg);  // cfg must outlive the simulation

    void update(double dt_seconds) override;
    const VehicleState& state() const override { return s_; }
    void apply(const Command& c) override;
    void setDriverInput(const DriverInput& in) override { in_ = in; }

private:
    void reset();
    void step(double dt);            // one fixed integration step
    void updateEcus();
    void updateTs(double dt);
    void updateAutonomous(double dt);
    void updatePedals(double dt);
    void updateInverters();
    void updateDriveline(double dt);
    void updateThermal(double dt);
    void updateAuxiliary(double dt);
    void updateEbs();
    void triggerEmergency();
    bool stationary() const;
    double missionTargetKmh() const;

    const Config& cfg_;
    VehicleState s_;
    DriverInput in_;
    double precharge_t_ = 0;
};

}  // namespace vds
