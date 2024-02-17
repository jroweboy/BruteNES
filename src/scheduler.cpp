

#include "scheduler.h"

void Scheduler::ScheduleInterrupt(s32 cycles_from_now, InterruptSource source, bool recurring) {
    queue.emplace(Interrupt{
        .cycles = cycle_count + cycles_from_now,
        .repeats = cycles_from_now,
        .source = source,
        .recurring = recurring
    });
}

s64 Scheduler::CPUCyclesTillInterrupt() {
    auto& top = queue.top();
    return ((s64)top.cycles - (s64)cycle_count) / NTSC_CPU_CLOCK;
}

void Scheduler::AddCPUCycles(s32 cycles) {
    auto i = queue.top();
    auto next_cycle_count = (cycle_count + cycles * NTSC_CLOCK_DIVIDER);
    while (i.cycles < next_cycle_count) {
        queue.pop();
        if (i.recurring) {
            queue.emplace(Interrupt{
                    .cycles = i.cycles + i.repeats,
                    .repeats = i.repeats,
                    .recurring = true
            });
        }
        i = queue.top();
        if (queue.empty())
            break;
    }
    cycle_count = next_cycle_count;
}

bool CompareInterrupt(Interrupt a, Interrupt b) {
    return a.cycles > b.cycles;
}
