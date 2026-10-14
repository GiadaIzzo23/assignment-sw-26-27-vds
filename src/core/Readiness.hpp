// "Can the autonomous system start?" as pure logic.
//
// Every prerequisite is in one of three states, because they mean different
// things to the person looking at the screen:
//   Done    - satisfied.
//   Pending - not yet done, and the operator can simply do it (press a key).
//   Blocked - cannot be done while something is wrong (a fault or a low
//             pressure): fixing the fault comes before anything else.
// The dashboard only decides how to draw these states; it does not decide them.
#pragma once

#include <array>
#include <string>

#include "core/VehicleState.hpp"
#include "core/Warnings.hpp"

namespace vds {

enum class CheckState { Done, Pending, Blocked };

struct ReadinessItem {
    const char* name = "";  // what the checklist shows, e.g. "ASMS on"
    CheckState state = CheckState::Pending;
    std::string blocker;    // why it is not done, e.g. "ASMS off"; empty when Done
};

constexpr int kReadinessItemCount = 6;

struct Readiness {
    std::array<ReadinessItem, kReadinessItemCount> items{};

    bool ready() const;       // every item is Done
    bool hasBlocked() const;  // at least one item is Blocked by a fault
    int notDone() const;      // number of Pending + Blocked items

    // The most important reason the system is not ready: the first Blocked item
    // if there is one (a fault outranks a missing step), otherwise the first
    // Pending item. nullptr when the system is ready.
    const ReadinessItem* firstBlocker() const;
};

// `tank_severity` is the severity of the low-pressure warning, which the warning
// engine computes (with hysteresis) from the configured thresholds.
Readiness evaluateReadiness(const VehicleState& s, Severity tank_severity);

}  // namespace vds
