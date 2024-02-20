

#include <fstream>
#include <istream>
#include <thread>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <argparse/argparse.hpp>

#include "brutenes.h"


consteval u32 RGB2Hex(std::array<u8, 3> rgb) {
    return 0xff << 24 | rgb[0] << 16 | rgb[1] << 8 | rgb[2] << 0;
}

constexpr std::array<std::array<u8, 3>, 0x40> BasePalette {{
    { 84,  84,  84}, {  0,  30, 116}, {  8,  16, 144}, { 48,   0, 136}, { 68,   0, 100}, { 92,   0,  48}, { 84,   4,   0}, { 60,  24,   0}, { 32,  42,   0}, {  8,  58,   0}, {  0,  64,   0}, {  0,  60,   0}, {  0,  50,  60}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
    {152, 150, 152}, {  8,  76, 196}, { 48,  50, 236}, { 92,  30, 228}, {136,  20, 176}, {160,  20, 100}, {152,  34,  32}, {120,  60,   0}, { 84,  90,   0}, { 40, 114,   0}, {  8, 124,   0}, {  0, 118,  40}, {  0, 102, 120}, {  0,   0,   0}, {  0,   0,   0}, {  0,   0,   0},
    {236, 238, 236}, { 76, 154, 236}, {120, 124, 236}, {176,  98, 236}, {228,  84, 236}, {236,  88, 180}, {236, 106, 100}, {212, 136,  32}, {160, 170,   0}, {116, 196,   0}, { 76, 208,  32}, { 56, 204, 108}, { 56, 180, 204}, { 60,  60,  60}, {  0,   0,   0}, {  0,   0,   0},
    {236, 238, 236}, {168, 204, 236}, {188, 188, 236}, {212, 178, 236}, {236, 174, 236}, {236, 174, 212}, {236, 180, 176}, {228, 196, 144}, {204, 210, 120}, {180, 222, 120}, {168, 226, 144}, {152, 226, 180}, {160, 214, 228}, {160, 162, 160}, {  0,   0,   0}, {  0,   0,   0},
}};

constexpr std::array<std::array<double, 3>, 8> EmphasisMultiplier {{
    {100.0,100.0,100.0},
    { 74.3, 91.5,123.9},
    { 88.2,108.6, 79.4},
    { 65.3, 98.0,101.9},
    {127.7,102.6, 90.5},
    { 97.9, 90.8,102.3},
    {100.1, 98.7, 74.1},
    { 75.0, 75.0, 75.0},
}};

consteval std::array<u32, 0x40 * 8> GenFullPalette() {
    std::array<u32, 0x40 * 8> out{};
    int i = 0;
    for (const auto& emp : EmphasisMultiplier) {
        for (const auto& pal : BasePalette) {
            std::array<u8, 3> mult = pal;
            mult[0] = (u8)std::clamp<double>(((double)mult[0] * emp[0] / 100.0), 0, 255);
            mult[1] = (u8)std::clamp<double>(((double)mult[1] * emp[1] / 100.0), 0, 255);
            mult[2] = (u8)std::clamp<double>(((double)mult[2] * emp[2] / 100.0), 0, 255);
            out[i] = RGB2Hex(mult);
            i++;
        }
    }
    return out;
}

constexpr std::array<u32, 0x40 * EmphasisMultiplier.size()> FullPalette = GenFullPalette();

class SDLFrontend {
public:
    SDLFrontend() {
        if (SDL_CreateWindowAndRenderer(320, 240, 0, &win, &renderer)) {
            return;
        }
        texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240 );
    }
    ~SDLFrontend() {
        if (texture)
            SDL_DestroyTexture(texture);
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (win)
            SDL_DestroyWindow(win);
    }
    void DrawFrame(u16* palette_buffer) {
        if (!renderer) return;
        for (int i = 0; i < 256 * 240; i++) {
            pixel_data[i] = FullPalette[palette_buffer[i]];
        }
        SDL_UpdateTexture(texture, nullptr, pixel_data.data(), 256 * sizeof (uint32_t));
        SDL_RenderClear(renderer); //clears the renderer
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer); //updates the renderer
    }
private:
    SDL_Window *win{};
    SDL_Renderer *renderer{};
    SDL_Texture *texture{};
    std::array<u32, 256 * 240> pixel_data{};
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
    frontend->DrawFrame(emu->GetFrame());

    SPDLOG_WARN("time taken {} FPS {}", emu->nes->last_timer,
                1000000000.0 / emu->nes->last_timer);
    return 0;
}

int SDL_AppEvent(const SDL_Event *event) {
    switch (event->type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        emu->Stop();
        return 1;
    default:
        return 0;
    }
}

void SDL_AppQuit(void) {
    emu->Stop();
    // Cleanup SDL resources
    frontend.reset();
}
