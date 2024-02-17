
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
#include "scheduler.h"

class BruteNES {
public:
    BruteNES() = delete;
    static std::unique_ptr<BruteNES> Init(std::vector<u8>&& romdata);

    void ColdBoot();
    void RunFrame();

private:
    explicit BruteNES(std::vector<u8>&& rom, std::span<u8> prg, std::span<u8> chr);
    Bus bus;
    CPU cpu;
    Scheduler timing;

    std::vector<u8> rom;
};

// Wraps the emu instance in a thread since macos still doesn't have jthread
class EmuThread {
public:
    static std::unique_ptr<EmuThread> Init(std::vector<u8>&& data);
    ~EmuThread();
    void Start();
    void Pause();
    void Stop();
private:
    EmuThread(std::unique_ptr<BruteNES>&& nes) : nes(std::move(nes)) {}

    std::unique_ptr<BruteNES> nes;
    std::thread th;
    std::atomic_bool stop_signal;
};

#endif //BRUTENES_BRUTENES_H
