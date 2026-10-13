// Keyboard mapping. Two separate channels, on purpose:
//  - readDriverInput(): continuous state of the held keys (pedals, steering),
//    sampled every frame -> DriverInput
//  - actionFor(): one-shot key presses -> Command (vehicle settings, faults...)
//    or a UI action (change mode, quit)
// The mapping is the only place that knows about SDL scancodes.
#pragma once

#include <SDL.h>

#include <optional>

#include "core/IVehicleSource.hpp"
#include "ui/Mode.hpp"

namespace vds {

struct KeyAction {
    std::optional<Command> command;  // forwarded to the vehicle source
    std::optional<Mode> mode;        // handled by the UI
    bool quit = false;
};

KeyAction actionFor(SDL_Scancode key);
DriverInput readDriverInput();

}  // namespace vds
