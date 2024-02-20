
#include <chrono>
#include <range/v3/view.hpp>

#include "brutenes.h"
#include "ines.h"

BruteNES::BruteNES(std::vector<u8>&& filedata, INES header, std::span<u8> prg, std::span<u8> chr)
    : bus(header, prg, chr, ppu), cpu(bus), ppu(bus, timing), header(header), timing(cpu), rom(filedata) {}

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

    return std::unique_ptr<BruteNES>(new BruteNES(std::move(filedata), *ines, prg, chr));
}

void BruteNES::RunFrame() {
    // run from the start of vblank until the start of the next vblank
    u32 current_frame = timing.frame_count;
    ppu.SwapBuffer();
    while (!stop_signal.load(std::memory_order_acquire)) {
        u32 estimate = timing.CPUCyclesTillInterrupt();
        bool use_ppu_cache = false;
        if (ppu.status.vblank) {
            // If we are in vblank, then its safe to use the write cache
            s64 estimate_adjustment = (s64)20 * PPU::CYCLES_PER_SCANLINE - (ppu.scanline * PPU::CYCLES_PER_SCANLINE + ppu.cycle);
            if (estimate_adjustment > 0) {
                estimate = estimate_adjustment / Scheduler::NTSC_CPU_CLOCK_DIVIDER;
                use_ppu_cache = true;
            }
        }
        u32 count = cpu.RunFor(estimate, use_ppu_cache);
        timing.AddCPUCycles(count);
        ppu.CatchUp();

        // Check for the IRQ flag to see if we need to run the 7 cycle IRQ handle
        if (cpu.pending_nmi) {
            if (ppu.ctrl.nmi != 0) {
                cpu.PushStack(cpu.PC >> 8);
                cpu.PushStack(cpu.PC & 0xff);
                cpu.PushStack(cpu.P);
                cpu.PC = bus.Read16(CPU::NMIVector);
                cpu.SetFlag<CPU::Flags::I>(true);
                timing.AddCPUCycles(7);
            }
            cpu.pending_nmi = false;
        }
        if (cpu.pending_irq && (cpu.P & CPU::Flags::I) == 0) {
            cpu.PushStack(cpu.PC >> 8);
            cpu.PushStack(cpu.PC & 0xff);
            cpu.PushStack(cpu.P);
            cpu.PC = bus.Read16(CPU::IRQVector);
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

u16* BruteNES::GetFrame() {
    if (paused) {
        return prev_frame;
    }
    prev_frame = ppu.LockBuffer();
    return prev_frame;
}

void BruteNES::RunLoop() {
    ColdBoot();
    while (!stop_signal.load(std::memory_order_acquire)) {
        if (paused) {
            std::unique_lock<std::mutex> lock(pause_mutex);
            pause_cv.wait(lock, [&]{ return !paused; });
        }

        typedef std::chrono::high_resolution_clock Clock;
        auto t1 = Clock::now();
        RunFrame();
        auto t2 = Clock::now();
        last_timer = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    }
}

EmuThread::~EmuThread() {
    if (th.joinable())
        th.join();
}

void EmuThread::Start() {
    th = std::thread([&]() {
        nes->RunLoop();
    });
}

void EmuThread::Stop() {
    nes->stop_signal.store(true, std::memory_order_release);
    if (th.joinable())
        th.join();
}

void EmuThread::Pause() {
    std::lock_guard<std::mutex> lk(nes->pause_mutex);
    nes->paused = !nes->paused;
}

std::unique_ptr<EmuThread> EmuThread::Init(std::vector<u8>&& data) {
    auto nes = BruteNES::Init(std::move(data));
    if (nes == nullptr) {
        return nullptr;
    }
    return std::unique_ptr<EmuThread>(new EmuThread(std::move(nes)));
}
