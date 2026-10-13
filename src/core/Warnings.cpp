#include "core/Warnings.hpp"

#include <algorithm>
#include <cstdio>

namespace vds {

const char* toString(Severity s) {
    switch (s) {
        case Severity::None: return "NONE";
        case Severity::Caution: return "CAUTION";
        case Severity::Warning: return "WARNING";
        case Severity::Critical: return "CRITICAL";
    }
    return "?";
}

Severity classifyRising(double v, const ThresholdSet& t) {
    if (v >= t.critical) return Severity::Critical;
    if (v >= t.warning) return Severity::Warning;
    if (v >= t.caution) return Severity::Caution;
    return Severity::None;
}

Severity classifyFalling(double v, const ThresholdSet& t) {
    if (v <= t.critical) return Severity::Critical;
    if (v <= t.warning) return Severity::Warning;
    if (v <= t.caution) return Severity::Caution;
    return Severity::None;
}

static const char* kInvNames[kInverterCount] = {"FL", "FR", "RL", "RR"};

std::string describe(const Warning& w) {
    char buf[96];
    switch (w.id) {
        case WarnId::InverterTemp:
            std::snprintf(buf, sizeof buf, "INV %s TEMP %.0f C", kInvNames[w.index], w.value);
            break;
        case WarnId::MotorTemp:
            std::snprintf(buf, sizeof buf, "MOTOR %s TEMP %.0f C", kInvNames[w.index], w.value);
            break;
        case WarnId::PackTemp:
            std::snprintf(buf, sizeof buf, "BATTERY TEMP %.0f C", w.value);
            break;
        case WarnId::LowSoc:
            std::snprintf(buf, sizeof buf, "LOW BATTERY %.0f %%", w.value);
            break;
        case WarnId::LowTankPressure:
            std::snprintf(buf, sizeof buf, "AIR TANK %.1f BAR", w.value);
            break;
        case WarnId::EcuStatus:
            std::snprintf(buf, sizeof buf, "%s %s", toString(static_cast<Ecu>(w.index)),
                          w.severity == Severity::Critical ? "FAULT" : "OFFLINE");
            break;
        case WarnId::EbsStatus:
            std::snprintf(buf, sizeof buf, "%s", w.severity == Severity::Critical
                                                     ? "EBS ACTIVATED / FAULT"
                                                     : "EBS UNAVAILABLE");
            break;
        case WarnId::AsEmergency:
            std::snprintf(buf, sizeof buf, "AS EMERGENCY");
            break;
    }
    return buf;
}

Severity WarningEngine::step(WarnId id, int index, Severity raw, double value, bool rising,
                             const ThresholdSet* t) {
    const auto key = std::make_pair(static_cast<int>(id), index);
    Severity prev = Severity::None;
    if (auto it = latched_.find(key); it != latched_.end()) prev = it->second;

    Severity result = raw;
    if (raw < prev && t) {
        // De-escalation: keep the old level until the value has recovered by the
        // hysteresis margin, i.e. classify against thresholds moved the "safe" way.
        ThresholdSet shifted = *t;
        const double h = t->hysteresis;
        const double dir = rising ? -h : +h;
        shifted.caution += dir;
        shifted.warning += dir;
        shifted.critical += dir;
        const Severity held = rising ? classifyRising(value, shifted) : classifyFalling(value, shifted);
        result = std::max(raw, std::min(prev, held));
    }
    if (result == Severity::None) latched_.erase(key);
    else latched_[key] = result;
    return result;
}

const std::vector<Warning>& WarningEngine::evaluate(const VehicleState& s) {
    active_.clear();
    auto push = [this](WarnId id, int idx, Severity sev, double v) {
        if (sev != Severity::None) active_.push_back({id, idx, sev, v});
    };
    const auto& th = cfg_.thresholds;

    for (int i = 0; i < kInverterCount; ++i) {
        const auto& inv = s.inverters[i];
        push(WarnId::InverterTemp, i,
             step(WarnId::InverterTemp, i, classifyRising(inv.temp_c, th.inverter_temp), inv.temp_c,
                  true, &th.inverter_temp),
             inv.temp_c);
        push(WarnId::MotorTemp, i,
             step(WarnId::MotorTemp, i, classifyRising(inv.motor_temp_c, th.motor_temp),
                  inv.motor_temp_c, true, &th.motor_temp),
             inv.motor_temp_c);
    }
    push(WarnId::PackTemp, 0,
         step(WarnId::PackTemp, 0, classifyRising(s.pack_temp_c, th.pack_temp), s.pack_temp_c, true,
              &th.pack_temp),
         s.pack_temp_c);
    push(WarnId::LowSoc, 0,
         step(WarnId::LowSoc, 0, classifyFalling(s.soc_pct, th.soc), s.soc_pct, false, &th.soc),
         s.soc_pct);

    // Tank pressure only matters when the autonomous system is switched on.
    {
        Severity raw = s.asms_on ? classifyFalling(s.tank_pressure_bar, th.tank_pressure)
                                 : Severity::None;
        push(WarnId::LowTankPressure, 0,
             step(WarnId::LowTankPressure, 0, raw, s.tank_pressure_bar, false, &th.tank_pressure),
             s.tank_pressure_bar);
    }

    for (int e = 0; e < kEcuCount; ++e) {
        Severity raw = Severity::None;
        if (s.ecu[e] == EcuStatus::Fault) raw = Severity::Critical;
        else if (s.ecu[e] == EcuStatus::Offline) raw = Severity::Warning;
        push(WarnId::EcuStatus, e, step(WarnId::EcuStatus, e, raw, 0, true, nullptr), 0);
    }

    {
        Severity raw = Severity::None;
        if (s.ebs == EbsState::Fault || s.ebs == EbsState::Activated) raw = Severity::Critical;
        else if (s.asms_on && s.ebs == EbsState::Unavailable) raw = Severity::Warning;
        push(WarnId::EbsStatus, 0, step(WarnId::EbsStatus, 0, raw, 0, true, nullptr), 0);
    }
    {
        Severity raw = s.as == AsState::Emergency ? Severity::Critical : Severity::None;
        push(WarnId::AsEmergency, 0, step(WarnId::AsEmergency, 0, raw, 0, true, nullptr), 0);
    }

    // Highest severity first; among equals the autonomous-system verdict beats its
    // consequences, which beat individual sensor readings.
    auto priority = [](WarnId id) {
        switch (id) {
            case WarnId::AsEmergency: return 3;
            case WarnId::EbsStatus: return 2;
            case WarnId::EcuStatus: return 1;
            default: return 0;
        }
    };
    std::stable_sort(active_.begin(), active_.end(), [&priority](const Warning& a, const Warning& b) {
        if (a.severity != b.severity) return a.severity > b.severity;
        return priority(a.id) > priority(b.id);
    });
    return active_;
}

Severity WarningEngine::worst() const {
    return active_.empty() ? Severity::None : active_.front().severity;
}

Severity WarningEngine::severityOf(WarnId id, int index) const {
    auto it = latched_.find({static_cast<int>(id), index});
    return it == latched_.end() ? Severity::None : it->second;
}

void WarningEngine::reset() {
    latched_.clear();
    active_.clear();
}

}  // namespace vds
