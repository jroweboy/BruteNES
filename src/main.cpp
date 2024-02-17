

#include <fstream>
#include <istream>
#include <thread>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <argparse/argparse.hpp>

#include "brutenes.h"


class SDLFrontend {
public:
    SDLFrontend() {
        if (SDL_CreateWindowAndRenderer(320, 240, 0, &win, &renderer)) {
            return;
        }
    }
    ~SDLFrontend() {
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (win)
            SDL_DestroyWindow(win);
    }
private:
    SDL_Window *win = nullptr;
    SDL_Renderer *renderer = nullptr;
};

static std::unique_ptr<SDLFrontend> frontend;
static std::unique_ptr<EmuThread> emu;

[[maybe_unused]] int SDL_AppInit(int argc, char **argv) {
    argparse::ArgumentParser program("brutenes");
    program.add_argument("romfile");
    program.add_argument("-s", "--save-state");
    
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    auto filename = program.get("romfile");
    frontend = std::make_unique<SDLFrontend>();

    std::ifstream instream(filename, std::ios::binary);
    std::vector<u8> file_contents((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());

    emu = EmuThread::Init(std::move(file_contents));
    if (!emu) {
        return 1;
    }

    emu->Start();

    return 0;
}

int SDL_AppIterate(void) {
    return 0;
}

int SDL_AppEvent(const SDL_Event *event) {
    return 0;
}

void SDL_AppQuit(void) {
    emu->Stop();
}
