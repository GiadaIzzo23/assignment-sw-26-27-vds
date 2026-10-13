// How a warning is PRESENTED depends on the active mode. Pure function, no SDL:
// detection lives in core/Warnings, drawing lives in ui/Pages, and this file is
// the (unit-tested) rule that connects them.
//
//  Debugging : everything is data. Cautions only tint the number; warnings and
//              criticals also appear as a chip in the alert strip.
//  Manual    : the driver sees nothing until it matters. Caution = small chip,
//              Warning = banner, Critical = flashing banner + flashing frame.
//              Secondary systems (tank, EBS, AS) stay hidden until critical.
//  Driverless: AS / EBS / tank / ECU problems are primary (banner, flashing
//              when critical). Thermal and SoC issues are secondary (chip).
#pragma once

#include "core/Warnings.hpp"
#include "ui/Mode.hpp"

namespace vds {

enum class Presentation { Hidden, Inline, Chip, Banner, FlashBanner };

inline Presentation presentationFor(Mode mode, WarnId id, Severity sev) {
    if (sev == Severity::None) return Presentation::Hidden;

    switch (mode) {
        case Mode::Debugging:
            return sev == Severity::Caution ? Presentation::Inline : Presentation::Chip;

        case Mode::Manual: {
            const bool secondary = id == WarnId::LowTankPressure || id == WarnId::EbsStatus ||
                                   id == WarnId::AsEmergency;
            if (secondary)
                return sev == Severity::Critical ? Presentation::Banner : Presentation::Hidden;
            if (sev == Severity::Caution) return Presentation::Chip;
            if (sev == Severity::Warning) return Presentation::Banner;
            return Presentation::FlashBanner;
        }

        case Mode::Driverless: {
            const bool primary = id == WarnId::LowTankPressure || id == WarnId::EbsStatus ||
                                 id == WarnId::AsEmergency || id == WarnId::EcuStatus;
            if (primary) {
                if (sev == Severity::Caution) return Presentation::Inline;
                if (sev == Severity::Warning) return Presentation::Banner;
                return Presentation::FlashBanner;
            }
            if (sev == Severity::Caution) return Presentation::Hidden;
            return sev == Severity::Warning ? Presentation::Chip : Presentation::Banner;
        }
    }
    return Presentation::Hidden;
}

// Blink phase derived from simulated time (2.5 Hz): deterministic and testable.
inline bool flashOn(double time_s) { return static_cast<long long>(time_s * 5.0) % 2 == 0; }

}  // namespace vds
