
#include <SDL3/SDL.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <argparse/argparse.hpp>

int SDL_AppInit(int argc, char **argv) {
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



    return 0;
}

int SDL_AppIterate(void) {
    return 0;
}

int SDL_AppEvent(const SDL_Event *event) {
    return 0;
}

void SDL_AppQuit(void) {
    return;
}
