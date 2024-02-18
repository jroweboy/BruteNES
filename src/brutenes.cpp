

#include <range/v3/view.hpp>

#include "brutenes.h"
#include "ines.h"

BruteNES::BruteNES(std::vector<u8>&& filedata, std::span<u8> prg, std::span<u8> chr)
    : bus(prg, chr), cpu(bus), ppu(bus, timing), timing(cpu), rom(filedata) {}

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
    u32 current_frame = timing.frame_count;
    while (true) {
        u32 count = cpu.RunFor(timing.CPUCyclesTillInterrupt());
        timing.AddCPUCycles(count);
        ppu.CatchUp();

        // Check for the IRQ flag to see if we need to run the 7 cycle IRQ handle
        if (cpu.pending_nmi) {
            if (ppu.ctrl.nmi != 0) {
                cpu.PushStack(cpu.PC >> 8);
                cpu.PushStack(cpu.PC & 0xff);
                cpu.PushStack(cpu.P);
                cpu.PC = bus.Read16(0xfffa);
                cpu.SetFlag<CPU::Flags::I>(true);
                timing.AddCPUCycles(7);
            }
            cpu.pending_nmi = false;
        }
        if (cpu.pending_irq && (cpu.P & CPU::Flags::I) == 0) {
            cpu.PushStack(cpu.PC >> 8);
            cpu.PushStack(cpu.PC & 0xff);
            cpu.PushStack(cpu.P);
            cpu.PC = bus.Read16(0xfffe);
            cpu.SetFlag<CPU::Flags::I>(true);
            timing.AddCPUCycles(7);
        }

        if (current_frame != timing.frame_count) {
            break;
        }
    }
}

void BruteNES::ColdBoot() {
    // Setup vblank interval timer
    timing.ScheduleInterrupt(Scheduler::NTSC_CLOCK_PER_FRAME, InterruptSource::NMI, true);
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
