
#ifndef BRUTENES_BRUTENES_H
#define BRUTENES_BRUTENES_H

#include <memory>
#include <optional>
#include <vector>
#include <thread>
#include <atomic>

#include "common.h"
#include "bus.h"
#include "cpu.h"
#include "ines.h"
#include "ppu.h"
#include "scheduler.h"
#include "controller.h"

class EmuThread;


class BruteNES {
public:
    // Reexport the controll button typedef
    using Button = Controller::Button;

    BruteNES() = delete;
    static std::unique_ptr<BruteNES> Init(std::vector<u8>&& romdata);

    void RunLoop();

    void ColdBoot();
    void RunFrame();

    u16* GetFrame();

    Controller controller1;
    Controller controller2;

private:
    explicit BruteNES(std::vector<u8>&& rom, INES header, std::span<u8> prg, std::span<u8> chr);
    Bus bus;
    CPU cpu;
    PPU ppu;
    INES header;
    Scheduler timing;

    std::vector<u8> rom;
    // When paused, keep the previous frame around so we can keep redrawing it
    u16* prev_frame{};

    std::atomic<bool> stop_signal{};
    bool paused = false;
    std::mutex pause_mutex{};
    std::condition_variable pause_cv{};
    long long last_timer{};

    std::mutex controller_mutex{};
friend class EmuThread;
friend class Controller;
};

// Wraps the emu instance in a thread since macos still doesn't have jthread
class EmuThread {
public:
    static std::unique_ptr<EmuThread> Init(std::vector<u8>&& data);
    ~EmuThread();
    void Start();
    void Pause();
    void Stop();
    void PressController(u8 controller, BruteNES::Button button) const;
    void ReleaseController(u8 controller, BruteNES::Button button) const;

    [[nodiscard]] u16* GetFrame() const {
        return nes->GetFrame();
    }

    std::unique_ptr<BruteNES> nes;

private:
    explicit EmuThread(std::unique_ptr<BruteNES>&& nes) : nes(std::move(nes)) {}

    std::thread th{};
};

#endif //BRUTENES_BRUTENES_H
