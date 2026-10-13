#include "check.hpp"
#include "core/Config.hpp"
#include "core/Simulation.hpp"
#include "core/Warnings.hpp"

using namespace vds;

namespace {

void run(Simulation& sim, double seconds) {
    for (double t = 0; t < seconds; t += 0.02) sim.update(0.02);
}

// TS on -> wait precharge -> hold brake -> ready to drive.
void enterDrive(Simulation& sim, const Config& cfg) {
    sim.apply({CommandType::TsToggle});
    run(sim, cfg.sim.precharge_time_s + 1.0);
    DriverInput braking;
    braking.brake = true;
    sim.setDriverInput(braking);
    run(sim, 0.5);
    sim.apply({CommandType::ReadyToDrive});
    run(sim, 0.2);
    sim.setDriverInput(DriverInput{});
}

}  // namespace

TEST(the_car_starts_switched_off_and_stationary) {
    Config cfg;
    Simulation sim(cfg);
    CHECK(sim.state().ts == TsState::Off);
    CHECK(sim.state().as == AsState::Off);
    CHECK_NEAR(sim.state().speed_kmh, 0.0, 1e-9);
    CHECK_EQ(sim.state().fault_mask, 0u);
}

TEST(the_tractive_system_goes_through_precharge_before_ready) {
    Config cfg;
    Simulation sim(cfg);
    sim.apply({CommandType::TsToggle});
    run(sim, 0.1);
    CHECK(sim.state().ts == TsState::Precharge);
    run(sim, cfg.sim.precharge_time_s + 1.0);
    CHECK(sim.state().ts == TsState::Ready);
}

TEST(throttle_is_ignored_while_the_tractive_system_is_off) {
    Config cfg;
    Simulation sim(cfg);
    DriverInput gas;
    gas.accelerate = true;
    sim.setDriverInput(gas);
    run(sim, 3.0);
    CHECK_NEAR(sim.state().speed_kmh, 0.0, 1e-6);
}

TEST(the_car_accelerates_in_drive_and_never_exceeds_max_speed) {
    Config cfg;
    Simulation sim(cfg);
    enterDrive(sim, cfg);
    CHECK(sim.state().ts == TsState::Drive);
    DriverInput gas;
    gas.accelerate = true;
    sim.setDriverInput(gas);
    run(sim, 5.0);
    CHECK(sim.state().speed_kmh > 10.0);
    run(sim, 60.0);
    CHECK(sim.state().speed_kmh <= cfg.sim.max_speed_kmh + 1e-6);
}

TEST(braking_slows_the_car_down) {
    Config cfg;
    Simulation sim(cfg);
    enterDrive(sim, cfg);
    DriverInput gas;
    gas.accelerate = true;
    sim.setDriverInput(gas);
    run(sim, 5.0);
    const double fast = sim.state().speed_kmh;
    DriverInput stop;
    stop.brake = true;
    sim.setDriverInput(stop);
    run(sim, 2.0);
    CHECK(sim.state().speed_kmh < fast);
}

TEST(the_inverter_overtemp_scenario_heats_up_and_raises_a_warning) {
    Config cfg;
    Simulation sim(cfg);
    WarningEngine engine(cfg);
    enterDrive(sim, cfg);
    sim.apply({CommandType::ToggleFault, Fault::InverterOverTemp});
    run(sim, 15.0);
    CHECK(hasFault(sim.state(), Fault::InverterOverTemp));
    CHECK(sim.state().inverters[0].temp_c > cfg.thresholds.inverter_temp.caution);
    engine.evaluate(sim.state());
    CHECK(engine.worst() >= Severity::Caution);
}

TEST(the_critical_soc_scenario_drains_the_battery_to_a_floor) {
    Config cfg;
    Simulation sim(cfg);
    WarningEngine engine(cfg);
    enterDrive(sim, cfg);
    sim.apply({CommandType::ToggleFault, Fault::CriticalSoc});
    run(sim, 40.0);
    CHECK(sim.state().soc_pct < cfg.thresholds.soc.caution);
    CHECK(sim.state().soc_pct >= 0.0);
    engine.evaluate(sim.state());
    CHECK(engine.severityOf(WarnId::LowSoc) >= Severity::Caution);
}

TEST(a_tank_leak_lowers_the_air_pressure) {
    Config cfg;
    Simulation sim(cfg);
    const double before = sim.state().tank_pressure_bar;
    sim.apply({CommandType::ToggleFault, Fault::TankLeak});
    run(sim, 5.0);
    CHECK(sim.state().tank_pressure_bar < before);
}

TEST(faults_toggle_and_can_all_be_cleared) {
    Config cfg;
    Simulation sim(cfg);
    sim.apply({CommandType::ToggleFault, Fault::BmsFault});
    CHECK(hasFault(sim.state(), Fault::BmsFault));
    sim.apply({CommandType::ToggleFault, Fault::BmsFault});
    CHECK(!hasFault(sim.state(), Fault::BmsFault));
    sim.apply({CommandType::ToggleFault, Fault::SmuLost});
    sim.apply({CommandType::ToggleFault, Fault::TankLeak});
    sim.apply({CommandType::ClearFaults});
    CHECK_EQ(sim.state().fault_mask, 0u);
}

TEST(reset_restores_the_initial_state) {
    Config cfg;
    Simulation sim(cfg);
    enterDrive(sim, cfg);
    sim.apply({CommandType::ToggleFault, Fault::TankLeak});
    run(sim, 2.0);
    sim.apply({CommandType::Reset});
    CHECK(sim.state().ts == TsState::Off);
    CHECK_EQ(sim.state().fault_mask, 0u);
    CHECK_NEAR(sim.state().speed_kmh, 0.0, 1e-9);
}

TEST(the_simulation_is_deterministic) {
    Config cfg;
    Simulation a(cfg);
    Simulation b(cfg);
    for (Simulation* s : {&a, &b}) {
        enterDrive(*s, cfg);
        DriverInput gas;
        gas.accelerate = true;
        s->setDriverInput(gas);
        run(*s, 4.0);
    }
    CHECK_NEAR(a.state().speed_kmh, b.state().speed_kmh, 1e-12);
    CHECK_NEAR(a.state().soc_pct, b.state().soc_pct, 1e-12);
}
