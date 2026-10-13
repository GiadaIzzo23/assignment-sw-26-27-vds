#include "core/Config.hpp"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace vds {

std::vector<std::pair<std::string, double*>> Config::fields() {
    std::vector<std::pair<std::string, double*>> table;

    auto add = [&table](const std::string& key, double& value) {
        table.emplace_back(key, &value);
    };
    auto add_set = [&add](const std::string& key, ThresholdSet& set) {
        add(key + ".caution", set.caution);
        add(key + ".warning", set.warning);
        add(key + ".critical", set.critical);
        add(key + ".hysteresis", set.hysteresis);
    };

    add_set("thresholds.inverter_temp", thresholds.inverter_temp);
    add_set("thresholds.motor_temp", thresholds.motor_temp);
    add_set("thresholds.pack_temp", thresholds.pack_temp);
    add_set("thresholds.soc", thresholds.soc);
    add_set("thresholds.tank_pressure", thresholds.tank_pressure);
    add("display.steer_full_deg", display.steer_full_deg);
    add("display.bar_full_bar", display.bar_full_bar);
    add("sim.max_speed_kmh", sim.max_speed_kmh);
    add("sim.max_accel_ms2", sim.max_accel_ms2);
    add("sim.max_brake_decel_ms2", sim.max_brake_decel_ms2);
    add("sim.drag_decel_ms2", sim.drag_decel_ms2);
    add("sim.regen_decel_per_level_ms2", sim.regen_decel_per_level_ms2);
    add("sim.ebs_decel_ms2", sim.ebs_decel_ms2);
    add("sim.stationary_kmh", sim.stationary_kmh);
    add("sim.lap_length_m", sim.lap_length_m);
    add("sim.max_step_s", sim.max_step_s);
    add("sim.pedal_rise_per_s", sim.pedal_rise_per_s);
    add("sim.pedal_fall_per_s", sim.pedal_fall_per_s);
    add("sim.pedal_deadband", sim.pedal_deadband);
    add("sim.steer_rate_deg_s", sim.steer_rate_deg_s);
    add("sim.steer_return_deg_s", sim.steer_return_deg_s);
    add("sim.steer_max_deg", sim.steer_max_deg);
    add("sim.power_levels_max", sim.power_levels_max);
    add("sim.regen_levels_max", sim.regen_levels_max);
    add("sim.tv_levels_max", sim.tv_levels_max);
    add("sim.drs_min_speed_kmh", sim.drs_min_speed_kmh);
    add("sim.drs_max_steer_deg", sim.drs_max_steer_deg);
    add("sim.motor_max_torque_nm", sim.motor_max_torque_nm);
    add("sim.motor_regen_torque_nm", sim.motor_regen_torque_nm);
    add("sim.motor_rpm_per_kmh", sim.motor_rpm_per_kmh);
    add("sim.max_power_kw", sim.max_power_kw);
    add("sim.regen_max_kw", sim.regen_max_kw);
    add("sim.lv_load_kw", sim.lv_load_kw);
    add("sim.derate_factor", sim.derate_factor);
    add("sim.pack_capacity_kwh", sim.pack_capacity_kwh);
    add("sim.pack_v_full", sim.pack_v_full);
    add("sim.pack_v_empty", sim.pack_v_empty);
    add("sim.pack_resistance_ohm", sim.pack_resistance_ohm);
    add("sim.ambient_c", sim.ambient_c);
    add("sim.inv_heat_c_per_s", sim.inv_heat_c_per_s);
    add("sim.inv_cool_per_s", sim.inv_cool_per_s);
    add("sim.motor_heat_c_per_s", sim.motor_heat_c_per_s);
    add("sim.motor_cool_per_s", sim.motor_cool_per_s);
    add("sim.pack_heat_c_per_s", sim.pack_heat_c_per_s);
    add("sim.pack_cool_per_s", sim.pack_cool_per_s);
    add("sim.fault_inv_heat_c_per_s", sim.fault_inv_heat_c_per_s);
    add("sim.fault_soc_drop_pct_per_s", sim.fault_soc_drop_pct_per_s);
    add("sim.fault_soc_floor_pct", sim.fault_soc_floor_pct);
    add("sim.tank_leak_bar_per_s", sim.tank_leak_bar_per_s);
    add("sim.precharge_time_s", sim.precharge_time_s);
    add("sim.go_delay_s", sim.go_delay_s);
    add("sim.tank_nominal_bar", sim.tank_nominal_bar);
    add("sim.tank_refill_bar_per_s", sim.tank_refill_bar_per_s);
    add("sim.tank_emergency_use_bar", sim.tank_emergency_use_bar);
    add("sim.brake_line_max_bar", sim.brake_line_max_bar);
    add("sim.auto_speed_slow_kmh", sim.auto_speed_slow_kmh);
    add("sim.auto_speed_normal_kmh", sim.auto_speed_normal_kmh);
    add("sim.auto_speed_fast_kmh", sim.auto_speed_fast_kmh);
    add("sim.auto_gain_ms", sim.auto_gain_ms);
    add("sim.auto_decel_ms2", sim.auto_decel_ms2);
    add("sim.auto_steer_amplitude_deg", sim.auto_steer_amplitude_deg);
    add("sim.auto_steer_rad_per_s", sim.auto_steer_rad_per_s);
    return table;
}

namespace {

std::string trim(const std::string& text) {
    const char* whitespace = " \t\r\n";
    const auto begin = text.find_first_not_of(whitespace);
    if (begin == std::string::npos) return "";
    const auto end = text.find_last_not_of(whitespace);
    return text.substr(begin, end - begin + 1);
}

}  // namespace

Config::LoadResult Config::loadFromString(const std::string& text) {
    LoadResult result;
    auto table = fields();
    std::istringstream input(text);
    std::string line;
    int line_number = 0;

    auto fail = [&](const std::string& message) {
        result.ok = false;
        result.errors.push_back("line " + std::to_string(line_number) + ": " + message);
    };

    while (std::getline(input, line)) {
        ++line_number;

        const auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        line = trim(line);
        if (line.empty()) continue;

        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            fail("expected 'key = value'");
            continue;
        }
        const std::string key = trim(line.substr(0, equals));
        const std::string value = trim(line.substr(equals + 1));

        double* target = nullptr;
        for (auto& entry : table) {
            if (entry.first == key) target = entry.second;
        }
        if (target == nullptr) {
            fail("unknown key '" + key + "'");
            continue;
        }

        char* end = nullptr;
        errno = 0;
        const double number = std::strtod(value.c_str(), &end);
        if (value.empty() || end == value.c_str() || *end != '\0' || errno != 0) {
            fail("invalid number '" + value + "' for key '" + key + "'");
            continue;
        }
        *target = number;
    }
    return result;
}

Config::LoadResult Config::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        LoadResult result;
        result.ok = false;
        result.errors.push_back("cannot open '" + path + "'");
        return result;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
}

}  // namespace vds
