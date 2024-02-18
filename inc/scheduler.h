

#ifndef BRUTENES_SCHEDULER_H
#define BRUTENES_SCHEDULER_H

#include <queue>

#include "common.h"

class CPU;

enum class InterruptSource {
    NMI,
    APU,
    External,
};

struct Interrupt {
    u64 cycles;
    s32 repeats;
    InterruptSource source;
    bool recurring;
};

bool CompareInterrupt(Interrupt a, Interrupt b);

class Scheduler {
public:
    // Number of master clock cycles in a frame
    constexpr static u32 NTSC_MASTER_CLOCK = 21477272; // cycles per second
    constexpr static double FRAMES_PER_SECOND = 60.0988118623;
    constexpr static u32 NTSC_CLOCK_PER_FRAME = (u32)(NTSC_MASTER_CLOCK / FRAMES_PER_SECOND); // cycles per frame
    static_assert(NTSC_CLOCK_PER_FRAME == 357366);
    constexpr static u32 NTSC_CPU_CLOCK_DIVIDER = 12;
    constexpr static u32 NTSC_PPU_CLOCK_DIVIDER = 4;
    constexpr static u32 NTSC_PPU_CLOCK = NTSC_CLOCK_PER_FRAME / NTSC_PPU_CLOCK_DIVIDER;
    constexpr static u32 NTSC_CPU_CLOCK = NTSC_CLOCK_PER_FRAME / NTSC_CPU_CLOCK_DIVIDER;

    Scheduler(CPU& cpu) : cpu(cpu) {}

    void ScheduleInterrupt(s32 cycle_count, InterruptSource source, bool recurring = false);

    [[nodiscard]] s64 CPUCyclesTillInterrupt() const;

    void AddCPUCycles(s64 cycle_count);

    u64 cycle_count{};

    u32 frame_count{};

    std::priority_queue<Interrupt, std::vector<Interrupt>, decltype(&CompareInterrupt)> queue{};

private:
    CPU& cpu;
};

#endif //BRUTENES_SCHEDULER_H
