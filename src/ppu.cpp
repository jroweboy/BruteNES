
#include "bus.h"
#include "scheduler.h"
#include "ppu.h"


void PPU::CatchUp() {
    s64 cycles_to_run = timing.cycle_count - current_cycle;
    constexpr auto CYCLES_PER_SCANLINE = 341 * Scheduler::NTSC_PPU_CLOCK_DIVIDER;
    while (cycles_to_run > 0) {
        if (cycles_to_run > CYCLES_PER_SCANLINE) {
            RunScanline();
            cycles_to_run -= CYCLES_PER_SCANLINE;
        }
    }
}

void PPU::Tick() {

}

void PPU::RunScanline() {
    // Cycle 0
    // Idle
//    current_cycle += 4;

    // Cycle 1 - 256
#pragma unroll
    for (int i = 0; i < 256; i++) {

    }

    // Cycle 257-320

    // Cycle 321-336

    // Cycle 337-340


}


