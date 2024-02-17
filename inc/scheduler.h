

#ifndef BRUTENES_SCHEDULER_H
#define BRUTENES_SCHEDULER_H

#include <queue>

#include "common.h"

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
    constexpr static u32 NTSC_MASTER_CLOCK = 21477272;
    constexpr static u32 NTSC_CPU_CLOCK_DIVIDER = 12;
    constexpr static u32 NTSC_PPU_CLOCK_DIVIDER = 4;
    constexpr static u32 NTSC_PPU_CLOCK = NTSC_MASTER_CLOCK / NTSC_PPU_CLOCK_DIVIDER;
    constexpr static u32 NTSC_CPU_CLOCK = NTSC_MASTER_CLOCK / NTSC_CPU_CLOCK_DIVIDER;

    void ScheduleInterrupt(s32 cycle_count, InterruptSource source, bool recurring = false);

    s64 CPUCyclesTillInterrupt();

    void AddCPUCycles(s64 cycle_count);

    u64 cycle_count{};

    std::priority_queue<Interrupt, std::vector<Interrupt>, decltype(&CompareInterrupt)> queue{};


};

#endif //BRUTENES_SCHEDULER_H
