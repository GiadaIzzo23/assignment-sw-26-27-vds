// Central place for every tunable number of the simulator.
// Nothing in the renderer or in the simulation logic contains a "magic number":
// it all comes from here.
//
// The defaults below can be overridden at run time with a plain-text file
// (see config/vds.cfg and Config::loadFromFile), so thresholds can be tuned
// without recompiling.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace vds {

// Three-level threshold with hysteresis.
//  - "rising" signals (temperatures) are bad when the value goes UP:
//        caution < warning < critical
//  - "falling" signals (SoC, pressure) are bad when the value goes DOWN:
//        caution > warning > critical
// The direction is decided by the warning logic, not stored here.
struct ThresholdSet {
    double caution;
    double warning;
    double critical;
    double hysteresis;  // margin the value must recover by before de-escalating
};

struct Config {
    // ------------------------------------------------------ warning thresholds
    struct Thresholds {
        ThresholdSet inverter_temp{70, 85, 100, 3};      // rising  [degC]
        ThresholdSet motor_temp{90, 110, 125, 3};        // rising  [degC]
        ThresholdSet pack_temp{42, 50, 58, 2};           // rising  [degC]
        ThresholdSet soc{30, 20, 10, 2};                 // falling [%]
        ThresholdSet tank_pressure{8.0, 7.0, 6.0, 0.3};  // falling [bar]
    } thresholds;

    // ---------------------------------------------------------------- loading
    // Every numeric field, addressable by a "section.name" key, for example
    // "thresholds.soc.critical". The pointers refer to this Config object.
    std::vector<std::pair<std::string, double*>> fields();

    struct LoadResult {
        bool ok = true;
        std::vector<std::string> errors;  // human readable, with line numbers
    };

    // Parses "key = value" lines ('#' starts a comment). Unknown keys and bad
    // numbers are reported in `errors` but do not abort loading: the valid
    // lines are still applied.
    LoadResult loadFromString(const std::string& text);
    LoadResult loadFromFile(const std::string& path);
};

}  // namespace vds
