// Central place for every tunable number of the simulator.
// Nothing in the renderer or in the simulation logic contains a "magic number":
// it all comes from here.
//
// The defaults below can be overridden at run time with a plain-text file
// (see config/vds.cfg and Config::loadFromFile), so thresholds can be tuned
// without recompiling.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace vds {

// Three-level threshold with hysteresis.
//  - "rising" signals (temperatures) are bad when the value goes UP:
//        caution < warning < critical
//  - "falling" signals (SoC, pressure) are bad when the value goes DOWN:
//        caution > warning > critical
// The direction is decided by the warning logic, not stored here.
struct ThresholdSet {
    double caution;
    double warning;
    double critical;
    double hysteresis;  // margin the value must recover by before de-escalating
};

struct Config {
    // ------------------------------------------------------ warning thresholds
    struct Thresholds {
        ThresholdSet inverter_temp{70, 85, 100, 3};      // rising  [degC]
        ThresholdSet motor_temp{90, 110, 125, 3};        // rising  [degC]
        ThresholdSet pack_temp{42, 50, 58, 2};           // rising  [degC]
        ThresholdSet soc{30, 20, 10, 2};                 // falling [%]
        ThresholdSet tank_pressure{8.0, 7.0, 6.0, 0.3};  // falling [bar]
    } thresholds;

    // ------------------------------------------------------ simulation model
    // Every constant used by Simulation. All doubles so they can be loaded
    // from the config file; "levels" are rounded to int where used.
    struct Sim {
        // limits and dynamics
        double max_speed_kmh = 120;
        double max_accel_ms2 = 8.0;
        double max_brake_decel_ms2 = 15.0;
        double drag_decel_ms2 = 0.4;
        double regen_decel_per_level_ms2 = 0.8;
        double ebs_decel_ms2 = 10.0;
        double stationary_kmh = 1.0;       // below this the car counts as stopped
        double lap_length_m = 1000;
        double max_step_s = 0.02;          // internal fixed integration step
        // driver inputs
        double pedal_rise_per_s = 4.0;
        double pedal_fall_per_s = 6.0;
        double pedal_deadband = 0.05;      // brake plausibility / lift-off
        double steer_rate_deg_s = 120;
        double steer_return_deg_s = 90;
        double steer_max_deg = 120;
        // driver settings ranges
        double power_levels_max = 5;       // power level 1..max
        double regen_levels_max = 4;       // regen level 0..max
        double tv_levels_max = 3;          // torque vectoring 0..max
        double drs_min_speed_kmh = 40;
        double drs_max_steer_deg = 15;
        // powertrain
        double motor_max_torque_nm = 21;
        double motor_regen_torque_nm = 10;
        double motor_rpm_per_kmh = 90;
        double max_power_kw = 80;
        double regen_max_kw = 30;
        double lv_load_kw = 0.3;
        double derate_factor = 0.5;        // torque factor of a derated inverter
        // accumulator
        double pack_capacity_kwh = 7.0;
        double pack_v_full = 600;
        double pack_v_empty = 450;
        double pack_resistance_ohm = 0.08;
        // thermal model (first order: dT/dt = heat*load - cool*(T - ambient))
        double ambient_c = 25;
        double inv_heat_c_per_s = 2.5;
        double inv_cool_per_s = 0.05;
        double motor_heat_c_per_s = 1.8;
        double motor_cool_per_s = 0.03;
        double pack_heat_c_per_s = 0.4;
        double pack_cool_per_s = 0.02;
        // injected scenarios
        double fault_inv_heat_c_per_s = 8.0;   // extra heat on inverter FL
        double fault_soc_drop_pct_per_s = 5.0;
        double fault_soc_floor_pct = 5.0;
        double tank_leak_bar_per_s = 0.25;
        // sequencing
        double precharge_time_s = 2.0;
        double go_delay_s = 5.0;
        // autonomous system
        double tank_nominal_bar = 10;
        double tank_refill_bar_per_s = 0.5;    // workshop refill, only when AS is Off
        double tank_emergency_use_bar = 1.0;
        double brake_line_max_bar = 60;
        double auto_speed_slow_kmh = 20;       // Inspection
        double auto_speed_normal_kmh = 40;     // Skidpad, Autocross, EBS test
        double auto_speed_fast_kmh = 60;       // Acceleration, Trackdrive
        double auto_gain_ms = 2.0;             // speed error (m/s) for full pedal
        double auto_decel_ms2 = 4.0;           // comfortable autonomous braking
        double auto_steer_amplitude_deg = 15;
        double auto_steer_rad_per_s = 0.5;
    } sim;

    // ---------------------------------------------------------------- loading
    // Every numeric field, addressable by a "section.name" key, for example
    // "thresholds.soc.critical". The pointers refer to this Config object.
    std::vector<std::pair<std::string, double*>> fields();

    struct LoadResult {
        bool ok = true;
        std::vector<std::string> errors;  // human readable, with line numbers
    };

    // Parses "key = value" lines ('#' starts a comment). Unknown keys and bad
    // numbers are reported in `errors` but do not abort loading: the valid
    // lines are still applied.
    LoadResult loadFromString(const std::string& text);
    LoadResult loadFromFile(const std::string& path);
};

}  // namespace vds
