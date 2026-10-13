// The three dashboard pages. They only READ VehicleState and the warning engine.
#pragma once

#include "core/VehicleState.hpp"
#include "core/Warnings.hpp"
#include "ui/Gfx.hpp"
#include "ui/Mode.hpp"

namespace vds {

struct Config;
void drawPage(Gfx& g, Mode mode, const VehicleState& s, const WarningEngine& warnings, const Config& cfg);

}  // namespace vds
