#include <cstddef>
#include <string>

#include "check.hpp"
#include "core/Config.hpp"
#include "core/VehicleState.hpp"
#include "core/Warnings.hpp"

using namespace vds;

// ========================================================= threshold evaluation
TEST(classify_rising_boundaries_are_inclusive) {
    ThresholdSet t{70, 85, 100, 3};
    CHECK(classifyRising(69.99, t) == Severity::None);
    CHECK(classifyRising(70.0, t) == Severity::Caution);
    CHECK(classifyRising(84.99, t) == Severity::Caution);
    CHECK(classifyRising(85.0, t) == Severity::Warning);
    CHECK(classifyRising(100.0, t) == Severity::Critical);
    CHECK(classifyRising(500.0, t) == Severity::Critical);
}

TEST(classify_falling_boundaries_are_inclusive) {
    ThresholdSet t{30, 20, 10, 2};
    CHECK(classifyFalling(30.01, t) == Severity::None);
    CHECK(classifyFalling(30.0, t) == Severity::Caution);
    CHECK(classifyFalling(20.0, t) == Severity::Warning);
    CHECK(classifyFalling(10.0, t) == Severity::Critical);
    CHECK(classifyFalling(0.0, t) == Severity::Critical);
}

TEST(engine_escalates_immediately) {
    Config cfg;
    WarningEngine eng(cfg);
    VehicleState s;
    s.inverters[2].temp_c = 101;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::InverterTemp, 2) == Severity::Critical);
    CHECK(eng.severityOf(WarnId::InverterTemp, 0) == Severity::None);
    CHECK(eng.worst() == Severity::Critical);
}

TEST(engine_hysteresis_prevents_flicker_rising) {
    Config cfg;  // inverter: warning 85, hysteresis 3
    WarningEngine eng(cfg);
    VehicleState s;
    s.inverters[0].temp_c = 86;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::InverterTemp, 0) == Severity::Warning);
    s.inverters[0].temp_c = 84;  // below threshold but inside the hysteresis band
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::InverterTemp, 0) == Severity::Warning);
    s.inverters[0].temp_c = 81.9;  // recovered by more than 3 degrees
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::InverterTemp, 0) == Severity::Caution);  // still >= caution (70)
    s.inverters[0].temp_c = 60;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::InverterTemp, 0) == Severity::None);
}

TEST(engine_hysteresis_falling_soc) {
    Config cfg;  // soc: warning 20, hysteresis 2
    WarningEngine eng(cfg);
    VehicleState s;
    s.soc_pct = 19;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::LowSoc) == Severity::Warning);
    s.soc_pct = 21;  // above 20 but within 2 % of it: stays
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::LowSoc) == Severity::Warning);
    s.soc_pct = 22.5;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::LowSoc) == Severity::Caution);
}

TEST(engine_critical_steps_down_one_level_at_a_time) {
    Config cfg;
    WarningEngine eng(cfg);
    VehicleState s;
    s.inverters[0].temp_c = 105;
    eng.evaluate(s);
    s.inverters[0].temp_c = 90;  // plain classification: Warning
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::InverterTemp, 0) == Severity::Warning);
}

TEST(engine_orders_by_severity) {
    Config cfg;
    WarningEngine eng(cfg);
    VehicleState s;
    s.soc_pct = 25;                 // caution
    s.inverters[1].temp_c = 90;     // warning
    s.pack_temp_c = 70;             // critical
    auto& w = eng.evaluate(s);
    CHECK_EQ(w.size(), 3u);
    CHECK(w[0].severity == Severity::Critical);
    CHECK(w[1].severity == Severity::Warning);
    CHECK(w[2].severity == Severity::Caution);
}

TEST(engine_puts_as_emergency_before_its_consequences_at_equal_severity) {
    Config cfg;
    WarningEngine eng(cfg);
    VehicleState s;
    s.asms_on = true;
    s.tank_pressure_bar = 1.0;       // critical
    s.ebs = EbsState::Activated;     // critical
    s.as = AsState::Emergency;       // critical
    auto& w = eng.evaluate(s);
    CHECK(w.size() >= 3);
    CHECK(w[0].id == WarnId::AsEmergency);
    CHECK(w[1].id == WarnId::EbsStatus);
}

TEST(engine_ignores_tank_pressure_when_asms_off) {
    Config cfg;
    WarningEngine eng(cfg);
    VehicleState s;
    s.tank_pressure_bar = 1.0;
    s.asms_on = false;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::LowTankPressure) == Severity::None);
    s.asms_on = true;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::LowTankPressure) == Severity::Critical);
}

TEST(engine_ecu_states_map_to_severity) {
    Config cfg;
    WarningEngine eng(cfg);
    VehicleState s;
    s.ecu[static_cast<int>(Ecu::Smu)] = EcuStatus::Offline;
    s.ecu[static_cast<int>(Ecu::Bms)] = EcuStatus::Fault;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::EcuStatus, static_cast<int>(Ecu::Smu)) == Severity::Warning);
    CHECK(eng.severityOf(WarnId::EcuStatus, static_cast<int>(Ecu::Bms)) == Severity::Critical);
    CHECK(eng.severityOf(WarnId::EcuStatus, static_cast<int>(Ecu::Vcu)) == Severity::None);
}

TEST(engine_uses_custom_thresholds_from_config) {
    Config cfg;
    cfg.thresholds.soc = {60, 50, 40, 1};  // someone tunes the config, no code change
    WarningEngine eng(cfg);
    VehicleState s;
    s.soc_pct = 55;
    eng.evaluate(s);
    CHECK(eng.severityOf(WarnId::LowSoc) == Severity::Caution);
}

TEST(engine_reset_clears_latched_state) {
    Config cfg;
    WarningEngine eng(cfg);
    VehicleState s;
    s.pack_temp_c = 70;
    eng.evaluate(s);
    eng.reset();
    CHECK(eng.worst() == Severity::None);
    CHECK(eng.severityOf(WarnId::PackTemp) == Severity::None);
}

TEST(describe_contains_the_value) {
    Warning w{WarnId::InverterTemp, 1, Severity::Warning, 87.0};
    CHECK(describe(w).find("FR") != std::string::npos);
    CHECK(describe(w).find("87") != std::string::npos);
}
