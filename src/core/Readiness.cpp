#include "core/Readiness.hpp"

namespace vds {

bool Readiness::ready() const {
    for (const ReadinessItem& item : items) {
        if (item.state != CheckState::Done) return false;
    }
    return true;
}

bool Readiness::hasBlocked() const {
    for (const ReadinessItem& item : items) {
        if (item.state == CheckState::Blocked) return true;
    }
    return false;
}

int Readiness::notDone() const {
    int count = 0;
    for (const ReadinessItem& item : items) {
        if (item.state != CheckState::Done) ++count;
    }
    return count;
}

const ReadinessItem* Readiness::firstBlocker() const {
    for (const ReadinessItem& item : items) {
        if (item.state == CheckState::Blocked) return &item;
    }
    for (const ReadinessItem& item : items) {
        if (item.state == CheckState::Pending) return &item;
    }
    return nullptr;
}

Readiness evaluateReadiness(const VehicleState& s, Severity tank_severity) {
    Readiness r;

    r.items[0] = {"ASMS on", s.asms_on ? CheckState::Done : CheckState::Pending,
                  s.asms_on ? "" : "ASMS off"};

    const bool has_mission = s.mission != Mission::None;
    r.items[1] = {"Mission selected", has_mission ? CheckState::Done : CheckState::Pending,
                  has_mission ? "" : "no mission"};

    // A BMS fault keeps the accumulator contactors open: the tractive system
    // cannot become ready until it is cleared.
    if (hasFault(s, Fault::BmsFault)) {
        r.items[2] = {"TS ready", CheckState::Blocked, "BMS fault"};
    } else if (s.ts == TsState::Ready || s.ts == TsState::Drive) {
        r.items[2] = {"TS ready", CheckState::Done, ""};
    } else {
        r.items[2] = {"TS ready", CheckState::Pending, "TS not ready"};
    }

    switch (s.ebs) {
        case EbsState::Armed:
            r.items[3] = {"EBS armed", CheckState::Done, ""};
            break;
        case EbsState::Unavailable:
            r.items[3] = {"EBS armed", CheckState::Pending, "EBS not armed"};
            break;
        case EbsState::Activated:
            r.items[3] = {"EBS armed", CheckState::Blocked, "EBS activated"};
            break;
        case EbsState::Fault:
            r.items[3] = {"EBS armed", CheckState::Blocked, "EBS fault"};
            break;
    }

    // An ECU that is not OK is a fault, never a step the operator can perform.
    r.items[4] = {"All ECUs OK", CheckState::Done, ""};
    for (int i = 0; i < kEcuCount; ++i) {
        if (s.ecu[i] != EcuStatus::Ok) {
            const Ecu ecu = static_cast<Ecu>(i);
            r.items[4] = {"All ECUs OK", CheckState::Blocked,
                          std::string(toString(ecu)) +
                              (s.ecu[i] == EcuStatus::Offline ? " offline" : " fault")};
            break;
        }
    }

    if (tank_severity >= Severity::Warning) {
        r.items[5] = {"Tank pressure OK", CheckState::Blocked, "low tank pressure"};
    } else {
        r.items[5] = {"Tank pressure OK", CheckState::Done, ""};
    }
    return r;
}

}  // namespace vds
