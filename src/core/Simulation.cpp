#include "core/Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace vds {
namespace {

constexpr double kTwoPi = 6.283185307179586;

int toInt(double v) { return static_cast<int>(std::lround(v)); }
double fraction(double value, double max) {
    return max > 0 ? std::clamp(value / max, 0.0, 1.0) : 0.0;
}
bool isAsActive(AsState a) { return a == AsState::Ready || a == AsState::Driving; }
bool tsLive(TsState t) { return t == TsState::Ready || t == TsState::Drive; }
double approach(double v, double target, double max_delta) {
    return v < target ? std::min(v + max_delta, target) : std::max(v - max_delta, target);
}

}  // namespace

Simulation::Simulation(const Config& cfg) : cfg_(cfg) { reset(); }

void Simulation::reset() {
    const auto& k = cfg_.sim;
    s_ = VehicleState{};
    s_.tank_pressure_bar = k.tank_nominal_bar;
    s_.pack_temp_c = k.ambient_c;
    for (auto& inv : s_.inverters) {
        inv.temp_c = k.ambient_c;
        inv.motor_temp_c = k.ambient_c;
    }
    precharge_t_ = 0;
    updateEcus();
    updateEbs();
    s_.pack_voltage = k.pack_v_empty + (k.pack_v_full - k.pack_v_empty) * s_.soc_pct / 100.0;
}

bool Simulation::stationary() const { return s_.speed_kmh <= cfg_.sim.stationary_kmh; }

// ------------------------------------------------------------------ commands
void Simulation::apply(const Command& cmd) {
    const auto& k = cfg_.sim;
    switch (cmd.type) {
        case CommandType::TsToggle:
            if (s_.ts == TsState::Off) {
                if (!hasFault(s_, Fault::BmsFault)) {
                    s_.ts = TsState::Precharge;
                    precharge_t_ = 0;
                }
            } else {
                s_.ts = TsState::Off;
            }
            break;
        case CommandType::ReadyToDrive:
            if (s_.ts == TsState::Ready && !s_.asms_on) s_.ts = TsState::Drive;
            break;
        case CommandType::PowerUp:
            s_.power_level = std::min(s_.power_level + 1, toInt(k.power_levels_max));
            break;
        case CommandType::PowerDown:
            s_.power_level = std::max(s_.power_level - 1, 1);
            break;
        case CommandType::RegenUp:
            s_.regen_level = std::min(s_.regen_level + 1, toInt(k.regen_levels_max));
            break;
        case CommandType::RegenDown:
            s_.regen_level = std::max(s_.regen_level - 1, 0);
            break;
        case CommandType::TvCycle:
            s_.tv_level = (s_.tv_level + 1) % (toInt(k.tv_levels_max) + 1);
            break;
        case CommandType::DrsToggle:
            s_.drs_requested = !s_.drs_requested;
            break;
        case CommandType::MissionNext:
        case CommandType::MissionPrev:
            if (s_.as == AsState::Off) {
                const int step = cmd.type == CommandType::MissionNext ? 1 : kMissionCount - 1;
                s_.mission = static_cast<Mission>((static_cast<int>(s_.mission) + step) % kMissionCount);
            }
            break;
        case CommandType::AsmsToggle:
            if (s_.asms_on) {
                s_.asms_on = false;
            } else if (stationary() && s_.ts != TsState::Drive) {
                s_.asms_on = true;
            }
            break;
        case CommandType::Go:
            if (s_.as == AsState::Ready && !s_.go_pending) {
                s_.go_pending = true;
                s_.go_countdown_s = k.go_delay_s;
            }
            break;
        case CommandType::EmergencyStop:
            if (isAsActive(s_.as)) {
                triggerEmergency();
            } else {
                s_.ts = TsState::Off;  // shutdown circuit opened by hand
            }
            break;
        case CommandType::MissionComplete:
            if (s_.as == AsState::Driving) {
                s_.as = AsState::Finished;
                s_.go_pending = false;
            }
            break;
        case CommandType::ToggleFault:
            s_.fault_mask ^= static_cast<std::uint32_t>(cmd.fault);
            break;
        case CommandType::ClearFaults:
            s_.fault_mask = 0;
            break;
        case CommandType::Reset:
            reset();
            break;
    }
}

// ------------------------------------------------------------------- stepping
void Simulation::update(double dt) {
    if (!(dt > 0)) return;  // also rejects NaN
    const double h = std::max(1e-3, cfg_.sim.max_step_s);
    double remaining = dt;
    while (remaining > 1e-12) {
        const double d = std::min(h, remaining);
        step(d);
        remaining -= d;
    }
}

void Simulation::step(double dt) {
    s_.time_s += dt;
    updateEcus();
    updateTs(dt);
    updateAutonomous(dt);
    updatePedals(dt);
    updateInverters();
    updateDriveline(dt);
    updateThermal(dt);
    updateAuxiliary(dt);
    updateEbs();
}

void Simulation::updateEcus() {
    auto at = [this](Ecu e) -> EcuStatus& { return s_.ecu[static_cast<std::size_t>(e)]; };
    s_.ecu.fill(EcuStatus::Ok);
    if (hasFault(s_, Fault::SmuLost)) at(Ecu::Smu) = EcuStatus::Offline;
    if (hasFault(s_, Fault::EbsFault)) at(Ecu::Ebs) = EcuStatus::Fault;
    if (hasFault(s_, Fault::BmsFault)) at(Ecu::Bms) = EcuStatus::Fault;
    if (hasFault(s_, Fault::AsFault)) at(Ecu::Asu) = EcuStatus::Fault;
    for (const auto& inv : s_.inverters) {
        if (inv.state == InverterState::Fault) at(Ecu::Vcu) = EcuStatus::Fault;
    }
}

void Simulation::updateTs(double dt) {
    if (hasFault(s_, Fault::BmsFault)) s_.ts = TsState::Off;  // contactors open
    if (s_.ts == TsState::Precharge) {
        precharge_t_ += dt;
        if (precharge_t_ >= cfg_.sim.precharge_time_s) s_.ts = TsState::Ready;
    }
}

void Simulation::triggerEmergency() {
    if (s_.as == AsState::Emergency) return;
    s_.as = AsState::Emergency;
    s_.ts = TsState::Off;  // shutdown circuit opens
    s_.go_pending = false;
    s_.go_countdown_s = 0;
    s_.tank_pressure_bar = std::max(0.0, s_.tank_pressure_bar - cfg_.sim.tank_emergency_use_bar);
}

double Simulation::missionTargetKmh() const {
    const auto& k = cfg_.sim;
    switch (s_.mission) {
        case Mission::None: return 0;
        case Mission::Inspection: return k.auto_speed_slow_kmh;
        case Mission::Skidpad:
        case Mission::Autocross:
        case Mission::EbsTest: return k.auto_speed_normal_kmh;
        case Mission::Acceleration:
        case Mission::Trackdrive: return k.auto_speed_fast_kmh;
    }
    return 0;
}

void Simulation::updateAutonomous(double dt) {
    // ASMS switched off: ends any non-emergency state; an emergency ends once stopped;
    // switching it off while driving is itself an emergency.
    if (!s_.asms_on) {
        if (s_.as == AsState::Driving) {
            triggerEmergency();
        } else if (s_.as == AsState::Emergency) {
            if (stationary()) s_.as = AsState::Off;
        } else if (s_.as != AsState::Off) {
            s_.as = AsState::Off;
            s_.go_pending = false;
            s_.go_countdown_s = 0;
        }
    }

    // Conditions that trigger an emergency while the AS is ready or driving.
    if (isAsActive(s_.as) &&
        (hasFault(s_, Fault::AsFault) || hasFault(s_, Fault::EbsFault) || !tsLive(s_.ts) ||
         s_.tank_pressure_bar <= cfg_.thresholds.tank_pressure.critical)) {
        triggerEmergency();
    }

    // Off -> Ready: ASMS on, mission chosen, EBS armed, no AS fault.
    if (s_.as == AsState::Off && s_.asms_on && s_.mission != Mission::None &&
        s_.ebs == EbsState::Armed && !hasFault(s_, Fault::AsFault)) {
        s_.as = AsState::Ready;
    }

    // Go countdown, then Ready -> Driving (the AS closes the "ready to drive" itself).
    if (s_.go_pending) {
        if (s_.as != AsState::Ready) {
            s_.go_pending = false;
            s_.go_countdown_s = 0;
        } else {
            s_.go_countdown_s -= dt;
            if (s_.go_countdown_s <= 0) {
                s_.go_countdown_s = 0;
                s_.go_pending = false;
                s_.as = AsState::Driving;
                s_.ts = TsState::Drive;
            }
        }
    }

    if (s_.as == AsState::Finished && stationary() && s_.ts != TsState::Off) s_.ts = TsState::Off;

    s_.target_speed_kmh = isAsActive(s_.as) ? missionTargetKmh() : 0.0;
}

void Simulation::updatePedals(double dt) {
    const auto& k = cfg_.sim;

    if (s_.as == AsState::Emergency) {  // EBS: no throttle, full braking, steering frozen
        s_.throttle = 0;
        s_.brake = 1;
        return;
    }

    if (s_.as == AsState::Driving || s_.as == AsState::Finished) {
        // Autonomous speed controller (proportional), synthesising pedal positions.
        const double err_ms = (s_.target_speed_kmh - s_.speed_kmh) / 3.6;
        s_.throttle = std::clamp(err_ms / k.auto_gain_ms, 0.0, 1.0);
        s_.brake = std::clamp(-err_ms / k.auto_gain_ms, 0.0, 1.0) *
                   (k.auto_decel_ms2 / k.max_brake_decel_ms2);
        const bool weaves = s_.mission == Mission::Skidpad || s_.mission == Mission::Autocross ||
                            s_.mission == Mission::Trackdrive;
        s_.steering_deg = (weaves && s_.as == AsState::Driving)
                              ? k.auto_steer_amplitude_deg * std::sin(k.auto_steer_rad_per_s * s_.time_s)
                              : 0.0;
        return;
    }

    // Manual: held keys move the pedals with finite slew rates.
    const double thr_rate = (in_.accelerate ? k.pedal_rise_per_s : k.pedal_fall_per_s) * dt;
    const double brk_rate = (in_.brake ? k.pedal_rise_per_s : k.pedal_fall_per_s) * dt;
    s_.throttle = approach(s_.throttle, in_.accelerate ? 1.0 : 0.0, thr_rate);
    s_.brake = approach(s_.brake, in_.brake ? 1.0 : 0.0, brk_rate);

    if (in_.steer != 0) {
        s_.steering_deg += in_.steer * k.steer_rate_deg_s * dt;
    } else {
        s_.steering_deg = approach(s_.steering_deg, 0.0, k.steer_return_deg_s * dt);
    }
    s_.steering_deg = std::clamp(s_.steering_deg, -k.steer_max_deg, k.steer_max_deg);
}

void Simulation::updateInverters() {
    const ThresholdSet& t = cfg_.thresholds.inverter_temp;
    for (auto& inv : s_.inverters) {
        InverterState base = InverterState::Off;
        if (s_.ts == TsState::Ready) base = InverterState::Idle;
        if (s_.ts == TsState::Drive) base = InverterState::Active;
        if (base == InverterState::Off) {
            inv.state = InverterState::Off;
            continue;
        }
        // Same hysteresis idea as the warnings: leave a state only after recovering.
        const bool was_fault = inv.state == InverterState::Fault;
        const bool was_hot = was_fault || inv.state == InverterState::Derated;
        const double crit_at = was_fault ? t.critical - t.hysteresis : t.critical;
        const double warn_at = was_hot ? t.warning - t.hysteresis : t.warning;
        if (inv.temp_c >= crit_at) {
            inv.state = InverterState::Fault;
        } else if (inv.temp_c >= warn_at && base == InverterState::Active) {
            inv.state = InverterState::Derated;
        } else {
            inv.state = base;
        }
    }
}

void Simulation::updateDriveline(double dt) {
    const auto& k = cfg_.sim;
    const bool drive = s_.ts == TsState::Drive;
    // Brake plausibility: pressing the brake cuts the throttle.
    const double thr = (drive && s_.brake <= k.pedal_deadband) ? s_.throttle : 0.0;
    const double vmax = k.max_speed_kmh / 3.6;
    double v = s_.speed_kmh / 3.6;
    const double power_frac = fraction(s_.power_level, k.power_levels_max);
    const double regen_frac = fraction(s_.regen_level, k.regen_levels_max);
    const bool lift = drive && thr <= k.pedal_deadband && v > 0.1;

    auto factor = [&k](InverterState st) {
        if (st == InverterState::Active) return 1.0;
        if (st == InverterState::Derated) return k.derate_factor;
        return 0.0;
    };

    // --- longitudinal dynamics (simplified: available torque scales the acceleration)
    double pf = 0;
    for (const auto& inv : s_.inverters) pf += factor(inv.state);
    pf /= kInverterCount;

    const double accel = thr * pf * power_frac * k.max_accel_ms2 * std::max(0.0, 1.0 - v / vmax);
    double decel = s_.brake * k.max_brake_decel_ms2;
    if (lift) decel += s_.regen_level * k.regen_decel_per_level_ms2;
    if (v > 0) decel += k.drag_decel_ms2;
    if (s_.as == AsState::Emergency) decel = k.ebs_decel_ms2;
    v = std::clamp(v + (accel - decel) * dt, 0.0, vmax);
    s_.speed_kmh = v * 3.6;
    s_.distance_m += v * dt;

    // --- per-inverter torque, power limit
    const double rpm = s_.speed_kmh * k.motor_rpm_per_kmh;
    const double omega = rpm * kTwoPi / 60.0;
    double pos_kw = 0, neg_kw = 0;
    for (auto& inv : s_.inverters) {
        const double f = factor(inv.state);
        double torque = thr * power_frac * f * k.motor_max_torque_nm;
        if (lift && f > 0) torque = -regen_frac * k.motor_regen_torque_nm;
        inv.torque_nm = torque;
        inv.rpm = rpm;
        const double kw = torque * omega / 1000.0;
        (kw >= 0 ? pos_kw : neg_kw) += std::fabs(kw);
    }
    const double pos_scale = pos_kw > k.max_power_kw ? k.max_power_kw / pos_kw : 1.0;
    const double neg_scale = neg_kw > k.regen_max_kw ? k.regen_max_kw / neg_kw : 1.0;
    double net_kw = 0;
    for (auto& inv : s_.inverters) {
        inv.torque_nm *= inv.torque_nm >= 0 ? pos_scale : neg_scale;
        net_kw += inv.torque_nm * omega / 1000.0;
    }

    // --- accumulator
    s_.pack_power_kw = tsLive(s_.ts) ? net_kw + k.lv_load_kw : 0.0;
    s_.soc_pct -= s_.pack_power_kw * dt / 3600.0 / k.pack_capacity_kwh * 100.0;
    if (hasFault(s_, Fault::CriticalSoc) && s_.soc_pct > k.fault_soc_floor_pct) {
        s_.soc_pct = std::max(k.fault_soc_floor_pct, s_.soc_pct - k.fault_soc_drop_pct_per_s * dt);
    }
    s_.soc_pct = std::clamp(s_.soc_pct, 0.0, 100.0);

    const double v_open = k.pack_v_empty + (k.pack_v_full - k.pack_v_empty) * s_.soc_pct / 100.0;
    s_.pack_current_a = v_open > 0 ? s_.pack_power_kw * 1000.0 / v_open : 0.0;
    s_.pack_voltage = v_open - s_.pack_current_a * k.pack_resistance_ohm;
}

void Simulation::updateThermal(double dt) {
    const auto& k = cfg_.sim;
    for (std::size_t i = 0; i < s_.inverters.size(); ++i) {
        auto& inv = s_.inverters[i];
        const double load = fraction(std::fabs(inv.torque_nm), k.motor_max_torque_nm);
        double heat = k.inv_heat_c_per_s * load;
        if (i == 0 && hasFault(s_, Fault::InverterOverTemp)) heat += k.fault_inv_heat_c_per_s;
        inv.temp_c += (heat - k.inv_cool_per_s * (inv.temp_c - k.ambient_c)) * dt;
        inv.motor_temp_c +=
            (k.motor_heat_c_per_s * load - k.motor_cool_per_s * (inv.motor_temp_c - k.ambient_c)) * dt;
    }
    const double pack_load = fraction(std::fabs(s_.pack_power_kw), k.max_power_kw);
    s_.pack_temp_c +=
        (k.pack_heat_c_per_s * pack_load - k.pack_cool_per_s * (s_.pack_temp_c - k.ambient_c)) * dt;
}

void Simulation::updateAuxiliary(double dt) {
    const auto& k = cfg_.sim;

    if (hasFault(s_, Fault::TankLeak)) {
        s_.tank_pressure_bar = std::max(0.0, s_.tank_pressure_bar - k.tank_leak_bar_per_s * dt);
    } else if (s_.as == AsState::Off) {
        s_.tank_pressure_bar =
            std::min(k.tank_nominal_bar, s_.tank_pressure_bar + k.tank_refill_bar_per_s * dt);
    }
    s_.brake_line_pressure_bar = s_.brake * k.brake_line_max_bar;

    // DRS opens only if requested, driving manually, fast enough, not braking, near-straight.
    s_.drs_open = s_.drs_requested && s_.ts == TsState::Drive && !s_.asms_on &&
                  s_.speed_kmh >= k.drs_min_speed_kmh && s_.brake < k.pedal_deadband &&
                  std::fabs(s_.steering_deg) <= k.drs_max_steer_deg;

    if (k.lap_length_m > 0) s_.lap = static_cast<int>(s_.distance_m / k.lap_length_m);
}

void Simulation::updateEbs() {
    if (hasFault(s_, Fault::EbsFault)) {
        s_.ebs = EbsState::Fault;
    } else if (s_.as == AsState::Emergency) {
        s_.ebs = EbsState::Activated;
    } else if (s_.asms_on && tsLive(s_.ts) &&
               s_.tank_pressure_bar > cfg_.thresholds.tank_pressure.critical) {
        s_.ebs = EbsState::Armed;
    } else {
        s_.ebs = EbsState::Unavailable;
    }
}

}  // namespace vds
