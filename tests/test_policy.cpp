#include "check.hpp"
#include "ui/WarningPolicy.hpp"

using namespace vds;

TEST(a_healthy_signal_is_never_shown) {
    for (Mode m : {Mode::Debugging, Mode::Manual, Mode::Driverless})
        CHECK(presentationFor(m, WarnId::PackTemp, Severity::None) == Presentation::Hidden);
}

TEST(the_same_overtemperature_looks_different_in_each_mode) {
    // Debugging: just a tinted number. Manual: flashing alert. Driverless: secondary chip/banner.
    CHECK(presentationFor(Mode::Debugging, WarnId::InverterTemp, Severity::Caution) == Presentation::Inline);
    CHECK(presentationFor(Mode::Manual, WarnId::InverterTemp, Severity::Critical) == Presentation::FlashBanner);
    CHECK(presentationFor(Mode::Driverless, WarnId::InverterTemp, Severity::Warning) == Presentation::Chip);
}

TEST(manual_mode_hides_secondary_systems_until_critical) {
    CHECK(presentationFor(Mode::Manual, WarnId::LowTankPressure, Severity::Warning) == Presentation::Hidden);
    CHECK(presentationFor(Mode::Manual, WarnId::LowTankPressure, Severity::Critical) == Presentation::Banner);
}

TEST(driverless_escalates_autonomous_faults) {
    CHECK(presentationFor(Mode::Driverless, WarnId::EbsStatus, Severity::Warning) == Presentation::Banner);
    CHECK(presentationFor(Mode::Driverless, WarnId::AsEmergency, Severity::Critical) == Presentation::FlashBanner);
}

TEST(flash_phase_follows_simulated_time) {
    CHECK(flashOn(0.0));
    CHECK(!flashOn(0.25));
    CHECK(flashOn(0.45));
}
