#include <cstddef>
#include <cstdint>
#include <string>

#include "check.hpp"
#include "core/Readiness.hpp"

using namespace vds;

namespace {

// A state in which every prerequisite is satisfied.
VehicleState readyState() {
    VehicleState s;
    s.asms_on = true;
    s.mission = Mission::Trackdrive;
    s.ts = TsState::Ready;
    s.ebs = EbsState::Armed;
    return s;
}

CheckState stateOf(const Readiness& r, int index) {
    return r.items[static_cast<std::size_t>(index)].state;
}

}  // namespace

TEST(readiness_a_fresh_car_has_four_steps_to_do_and_no_fault) {
    const Readiness r = evaluateReadiness(VehicleState{}, Severity::None);
    CHECK(!r.ready());
    CHECK(!r.hasBlocked());
    CHECK_EQ(r.notDone(), 4);
    CHECK(stateOf(r, 0) == CheckState::Pending);  // ASMS
    CHECK(stateOf(r, 1) == CheckState::Pending);  // mission
    CHECK(stateOf(r, 2) == CheckState::Pending);  // TS
    CHECK(stateOf(r, 3) == CheckState::Pending);  // EBS
    CHECK(stateOf(r, 4) == CheckState::Done);     // ECUs
    CHECK(stateOf(r, 5) == CheckState::Done);     // tank
    CHECK(r.firstBlocker() != nullptr);
    CHECK_EQ(r.firstBlocker()->blocker, std::string("ASMS off"));
}

TEST(readiness_everything_satisfied_means_ready) {
    const Readiness r = evaluateReadiness(readyState(), Severity::None);
    CHECK(r.ready());
    CHECK(!r.hasBlocked());
    CHECK_EQ(r.notDone(), 0);
    CHECK(r.firstBlocker() == nullptr);
}

TEST(readiness_ts_in_drive_also_counts_as_ready) {
    VehicleState s = readyState();
    s.ts = TsState::Drive;
    CHECK(evaluateReadiness(s, Severity::None).ready());
    s.ts = TsState::Precharge;  // still charging: a step in progress, not a fault
    const Readiness r = evaluateReadiness(s, Severity::None);
    CHECK(stateOf(r, 2) == CheckState::Pending);
}

TEST(readiness_an_ecu_fault_is_blocked_not_pending_and_names_the_ecu) {
    VehicleState s = readyState();
    s.ecu[static_cast<int>(Ecu::Asu)] = EcuStatus::Fault;
    Readiness r = evaluateReadiness(s, Severity::None);
    CHECK(!r.ready());
    CHECK(r.hasBlocked());
    CHECK(stateOf(r, 4) == CheckState::Blocked);
    CHECK_EQ(r.items[4].blocker, std::string("ASU fault"));

    s.ecu[static_cast<int>(Ecu::Asu)] = EcuStatus::Ok;
    s.ecu[static_cast<int>(Ecu::Smu)] = EcuStatus::Offline;
    r = evaluateReadiness(s, Severity::None);
    CHECK_EQ(r.items[4].blocker, std::string("SMU offline"));
}

TEST(readiness_ebs_unavailable_is_a_step_but_fault_and_activated_are_blocked) {
    VehicleState s = readyState();
    s.ebs = EbsState::Unavailable;
    CHECK(stateOf(evaluateReadiness(s, Severity::None), 3) == CheckState::Pending);
    s.ebs = EbsState::Fault;
    Readiness r = evaluateReadiness(s, Severity::None);
    CHECK(stateOf(r, 3) == CheckState::Blocked);
    CHECK_EQ(r.items[3].blocker, std::string("EBS fault"));
    s.ebs = EbsState::Activated;
    r = evaluateReadiness(s, Severity::None);
    CHECK(stateOf(r, 3) == CheckState::Blocked);
    CHECK_EQ(r.items[3].blocker, std::string("EBS activated"));
}

TEST(readiness_a_bms_fault_blocks_the_tractive_system_even_if_it_looks_ready) {
    VehicleState s = readyState();
    s.fault_mask = static_cast<std::uint32_t>(Fault::BmsFault);
    const Readiness r = evaluateReadiness(s, Severity::None);
    CHECK(stateOf(r, 2) == CheckState::Blocked);
    CHECK_EQ(r.items[2].blocker, std::string("BMS fault"));
    CHECK(!r.ready());
}

TEST(readiness_tank_pressure_blocks_from_warning_level_up) {
    const VehicleState s = readyState();
    CHECK(evaluateReadiness(s, Severity::Caution).ready());  // caution does not block
    Readiness r = evaluateReadiness(s, Severity::Warning);
    CHECK(stateOf(r, 5) == CheckState::Blocked);
    CHECK_EQ(r.items[5].blocker, std::string("low tank pressure"));
    CHECK(evaluateReadiness(s, Severity::Critical).hasBlocked());
}

TEST(readiness_a_fault_outranks_a_missing_step_as_the_main_blocker) {
    VehicleState s;  // fresh car: ASMS off is the first pending step
    s.ecu[static_cast<int>(Ecu::Asu)] = EcuStatus::Fault;
    const Readiness r = evaluateReadiness(s, Severity::None);
    CHECK(r.firstBlocker() != nullptr);
    CHECK_EQ(r.firstBlocker()->blocker, std::string("ASU fault"));
    CHECK_EQ(r.notDone(), 5);  // four steps plus the fault
}
