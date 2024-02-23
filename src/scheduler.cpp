
#include "cpu.h"
#include "scheduler.h"

void Scheduler::ScheduleInterrupt(s32 cycles_from_now, InterruptSource source, bool recurring) {
    queue.emplace(Interrupt{
        .cycles = cycle_count + cycles_from_now,
        .repeats = cycles_from_now,
        .source = source,
        .recurring = recurring
    });
}

s64 Scheduler::CPUCyclesTillInterrupt() const {
    auto& top = queue.top();
    return ((s64)top.cycles - (s64)cycle_count) / NTSC_CPU_CLOCK_DIVIDER;
}

void Scheduler::AddCPUCycles(s64 cycles) {
    auto i = queue.top();
    auto next_cycle_count = (cycle_count + cycles * NTSC_CPU_CLOCK_DIVIDER);
    while (i.cycles < next_cycle_count) {
        queue.pop();
        if (i.source == InterruptSource::NMI) {
            cpu.pending_nmi = true;
        } else {
            cpu.pending_irq = true;
        }
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
    cycle_count += cycles;
    frame_count = cycle_count / NTSC_CLOCK_PER_FRAME;
}

bool CompareInterrupt(const Interrupt& a, const Interrupt& b) {
    return a.cycles > b.cycles;
}
