
#include "bus.h"
#include "scheduler.h"
#include "ppu.h"


void PPU::CatchUp() {
    s64 cycles_to_run = timing.cycle_count - current_cycle;
    while (cycles_to_run > 0) {
        if (cycles_to_run > CYCLES_PER_SCANLINE) {
            RunScanline();
            cycles_to_run -= CYCLES_PER_SCANLINE;
        } else {
            Tick();
        }
        cycles_to_run = timing.cycle_count - current_cycle;
    }
}

void PPU::Tick() {
    current_cycle += 4;
}

void PPU::RunScanline() {
    // Cycle 0
    // Idle
//    current_cycle += 4;
    current_cycle += CYCLES_PER_SCANLINE;

    // Cycle 1 - 256
#pragma unroll
    for (int i = 0; i < 256; i++) {

    }

    // Cycle 257-320

    // Cycle 321-336

    // Cycle 337-340


}


