// Entry point: owns the window, the clock and the keyboard. It wires the pieces
// together and nothing else:
//   keyboard -> Command / DriverInput -> IVehicleSource -> VehicleState -> pages
#include <SDL.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "core/Config.hpp"
#include "core/Simulation.hpp"
#include "ui/Gfx.hpp"
#include "ui/Input.hpp"
#include "core/Warnings.hpp"
#include "ui/Mode.hpp"
#include "ui/Pages.hpp"

namespace {

constexpr int kCanvasW = 1280;
constexpr int kCanvasH = 720;
constexpr double kMaxFrameS = 0.1;  // never simulate more than this per frame

}  // namespace

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Assets and config are copied next to the executable by the build.
    char* base_raw = SDL_GetBasePath();
    const std::string base = base_raw ? base_raw : "./";
    SDL_free(base_raw);

    vds::Config cfg;
    const std::string cfg_path = argc > 1 ? argv[1] : base + "config/vds.cfg";
    const auto loaded = cfg.loadFromFile(cfg_path);
    for (const auto& e : loaded.errors) std::fprintf(stderr, "config: %s\n", e.c_str());
    if (!loaded.ok) std::fprintf(stderr, "config: using built-in defaults where needed\n");

    int exit_code = 0;
    {
        vds::Gfx gfx;
        std::string error;
        if (!gfx.init(base + "assets/fonts/DejaVuSansMono.ttf", "Vehicle Dashboard Simulator",
                      kCanvasW, kCanvasH, error)) {
            std::fprintf(stderr, "%s\n", error.c_str());
            exit_code = 1;
        } else {
            vds::Simulation simulation(cfg);
            // From here on the UI only knows the interface: swapping the
            // simulation for a real data stream does not touch anything below.
            vds::IVehicleSource& source = simulation;

            vds::WarningEngine warnings(cfg);
            vds::Mode mode = vds::Mode::Manual;
            bool running = true;
            Uint64 previous = SDL_GetPerformanceCounter();

            while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        running = false;
                    } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                        const vds::KeyAction action = vds::actionFor(event.key.keysym.scancode);
                        if (action.quit) running = false;
                        if (action.mode) mode = *action.mode;
                        if (action.command) source.apply(*action.command);
                    }
                }
                source.setDriverInput(vds::readDriverInput());

                const Uint64 now = SDL_GetPerformanceCounter();
                const double dt = static_cast<double>(now - previous) /
                                  static_cast<double>(SDL_GetPerformanceFrequency());
                previous = now;
                source.update(std::min(dt, kMaxFrameS));

                warnings.evaluate(source.state());
                gfx.beginFrame({12, 14, 20});
                vds::drawPage(gfx, mode, source.state(), warnings);
                gfx.endFrame();
            }
        }
    }
    SDL_Quit();
    return exit_code;
}
