#include <cstdint>
#include <string>

#include "check.hpp"
#include "core/VehicleState.hpp"

using namespace vds;

// ---- VehicleState: the plain data the dashboard reads -----------------------

TEST(default_state_is_a_parked_car) {
    const VehicleState s;
    CHECK_NEAR(s.speed_kmh, 0.0, 1e-12);
    CHECK_NEAR(s.soc_pct, 100.0, 1e-12);
    CHECK(s.ts == TsState::Off);
    CHECK(s.as == AsState::Off);
    CHECK(s.ebs == EbsState::Unavailable);
    CHECK(s.mission == Mission::None);
    CHECK_EQ(s.fault_mask, 0u);
    CHECK_EQ(s.inverters.size(), static_cast<std::size_t>(kInverterCount));
}

TEST(fault_bits_are_distinct_and_testable) {
    VehicleState s;
    for (int i = 0; i < kFaultCount; ++i) CHECK(!hasFault(s, faultFromIndex(i)));

    s.fault_mask = static_cast<std::uint32_t>(Fault::CriticalSoc) |
                   static_cast<std::uint32_t>(Fault::TankLeak);
    CHECK(hasFault(s, Fault::CriticalSoc));
    CHECK(hasFault(s, Fault::TankLeak));
    CHECK(!hasFault(s, Fault::InverterOverTemp));

    // Every fault index must map to its own single bit, otherwise two scenarios
    // would switch on together.
    std::uint32_t seen = 0;
    for (int i = 0; i < kFaultCount; ++i) {
        const auto bit = static_cast<std::uint32_t>(faultFromIndex(i));
        CHECK((seen & bit) == 0u);
        seen |= bit;
    }
}

TEST(ecu_status_is_looked_up_by_ecu) {
    VehicleState s;
    s.ecu[static_cast<int>(Ecu::Bms)] = EcuStatus::Fault;
    CHECK(ecuStatus(s, Ecu::Bms) == EcuStatus::Fault);
    CHECK(ecuStatus(s, Ecu::Smu) == EcuStatus::Ok);
}

TEST(every_enum_value_has_a_readable_name) {
    CHECK_EQ(std::string(toString(TsState::Drive)), std::string("DRIVE"));
    CHECK_EQ(std::string(toString(Ecu::Ebs)), std::string("EBS"));

    // A new enum value that is added without a name would show up as "?".
    for (int i = 0; i < kMissionCount; ++i) {
        CHECK(std::string(toString(static_cast<Mission>(i))) != "?");
    }
    for (int i = 0; i < kEcuCount; ++i) {
        CHECK(std::string(toString(static_cast<Ecu>(i))) != "?");
    }
    for (int i = 0; i < kFaultCount; ++i) {
        CHECK(std::string(toString(faultFromIndex(i))) != "?");
    }
}
