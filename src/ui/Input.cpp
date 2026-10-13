#include "ui/Input.hpp"

namespace vds {

KeyAction actionFor(SDL_Scancode key) {
    KeyAction a;
    auto cmd = [&a](CommandType t) { a.command = Command{t}; };

    // F1..F7 toggle the injectable faults, in the order of the Fault enum.
    if (key >= SDL_SCANCODE_F1 && key < SDL_SCANCODE_F1 + kFaultCount) {
        a.command = Command{CommandType::ToggleFault, faultFromIndex(key - SDL_SCANCODE_F1)};
        return a;
    }

    switch (key) {
        case SDL_SCANCODE_ESCAPE: a.quit = true; break;

        case SDL_SCANCODE_1: case SDL_SCANCODE_KP_1: case SDL_SCANCODE_I: a.mode = Mode::Debugging; break;
        case SDL_SCANCODE_2: case SDL_SCANCODE_KP_2: case SDL_SCANCODE_O: a.mode = Mode::Manual; break;
        case SDL_SCANCODE_3: case SDL_SCANCODE_KP_3: case SDL_SCANCODE_P: a.mode = Mode::Driverless; break;

        case SDL_SCANCODE_T: cmd(CommandType::TsToggle); break;
        case SDL_SCANCODE_RETURN: cmd(CommandType::ReadyToDrive); break;
        case SDL_SCANCODE_SPACE: cmd(CommandType::DrsToggle); break;
        case SDL_SCANCODE_Z: cmd(CommandType::PowerDown); break;
        case SDL_SCANCODE_X: cmd(CommandType::PowerUp); break;
        case SDL_SCANCODE_C: cmd(CommandType::RegenDown); break;
        case SDL_SCANCODE_V: cmd(CommandType::RegenUp); break;
        case SDL_SCANCODE_B: cmd(CommandType::TvCycle); break;

        case SDL_SCANCODE_M: cmd(CommandType::AsmsToggle); break;
        case SDL_SCANCODE_Q: cmd(CommandType::MissionPrev); break;
        case SDL_SCANCODE_E: cmd(CommandType::MissionNext); break;
        case SDL_SCANCODE_G: cmd(CommandType::Go); break;
        case SDL_SCANCODE_K: cmd(CommandType::MissionComplete); break;
        case SDL_SCANCODE_BACKSPACE: cmd(CommandType::EmergencyStop); break;

        case SDL_SCANCODE_F8: cmd(CommandType::ClearFaults); break;
        case SDL_SCANCODE_F9: cmd(CommandType::Reset); break;
        default: break;
    }
    return a;
}

DriverInput readDriverInput() {
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    const bool right = k[SDL_SCANCODE_D] != 0 || k[SDL_SCANCODE_RIGHT] != 0;
    const bool left = k[SDL_SCANCODE_A] != 0 || k[SDL_SCANCODE_LEFT] != 0;

    DriverInput in;
    in.accelerate = k[SDL_SCANCODE_W] != 0 || k[SDL_SCANCODE_UP] != 0;
    in.brake = k[SDL_SCANCODE_S] != 0 || k[SDL_SCANCODE_DOWN] != 0;
    in.steer = (right ? 1 : 0) - (left ? 1 : 0);
    return in;
}

}  // namespace vds
