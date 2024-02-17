

#include <range/v3/view.hpp>

#include "brutenes.h"
#include "ines.h"

BruteNES::BruteNES(std::vector<u8>&& filedata, std::span<u8> prg, std::span<u8> chr)
    : bus(prg, chr), cpu(bus), timing(), rom(filedata) {}

std::unique_ptr<BruteNES> BruteNES::Init(std::vector<u8>&& filedata) {
    auto ines = INES::ParseHeader(std::span(filedata).subspan(0, 0x10));
    if (!ines) {
        // INES header not valid
        return nullptr;
    }
    if (filedata.size() - 0x10 != ines->prg_size + ines->chr_size) {
        return nullptr;
    }
    auto prg = std::span(filedata).subspan(0x10, ines->prg_size);
    auto chr = std::span(filedata).subspan(0x10 + ines->prg_size, ines->chr_size);

    return std::unique_ptr<BruteNES>(new BruteNES(std::move(filedata), prg, chr));
}

void BruteNES::RunFrame() {
    // run from the start of vblank until the start of the next vblank
    while (true) {
        auto& next_interrupt = timing.queue.top();
        cpu.RunFor(timing.CPUCyclesTillInterrupt());
        break;
    }
}

void BruteNES::ColdBoot() {
    // Setup vblank interval timer
    timing.ScheduleInterrupt(Scheduler::NTSC_MASTER_CLOCK, InterruptSource::NMI, true);
    cpu.Reset();
}

EmuThread::~EmuThread() {
    th.join();
}

void EmuThread::Start() {
    th = std::thread([&]() {
        nes->ColdBoot();
        while (!stop_signal.load()) {
            nes->RunFrame();
        }
    });
}

void EmuThread::Stop() {

}

void EmuThread::Pause() {

}

std::unique_ptr<EmuThread> EmuThread::Init(std::vector<u8>&& data) {
    auto nes = BruteNES::Init(std::move(data));
    if (nes == nullptr) {
        return nullptr;
    }
    return std::unique_ptr<EmuThread>(new EmuThread(std::move(nes)));
}
