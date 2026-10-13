// The three dashboard contexts. Lives in the UI layer: the vehicle model does
// not know (and must not care) which page is being shown.
#pragma once

namespace vds {

enum class Mode { Debugging, Manual, Driverless };

inline const char* toString(Mode m) {
    switch (m) {
        case Mode::Debugging: return "DEBUGGING";
        case Mode::Manual: return "MANUAL DRIVING";
        case Mode::Driverless: return "DRIVERLESS";
    }
    return "?";
}

}  // namespace vds
