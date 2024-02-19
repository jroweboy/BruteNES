
#include "bus.h"
#include "scheduler.h"
#include "ppu.h"

u8 PPU::ReadRegister(u16 addr) {
    switch (addr & 0b0000'0111) {
    case 2:
        w = false;
        status.openbus = latch;
        return status.raw;
    default:
    case 0:
        return latch;
    }
}

void PPU::WriteRegister(u16 addr, u8 value) {
    switch (addr & 0b0000'0111) {
    case 0:
        ctrl.raw = value;
        // Set bits 10-11 of t with the nametable
        t = (t & ~(0b11 << 10)) | ctrl.nametable << 10;
        break;
    case 1:
        mask.raw = value;
        break;
    case 3:
        oamaddr = value;
        break;
    case 4:
        if (scanline < 240 && RenderingEnabled()) {
            //"Writes to OAMDATA during rendering (on the pre-render line and the visible lines 0-239, provided either sprite or background rendering is enabled) do not modify values in OAM,
            //but do perform a glitchy vertical_write of OAMADDR, bumping only the high 6 bits"
            oamaddr += 4;
        } else {
            //"The three unimplemented bits of each sprite's byte 2 do not exist in the PPU and always read back as 0 on PPU revisions that allow reading PPU OAM through OAMDATA ($2004)"
            if((oamaddr & 0x03) == 0x02) {
                value &= 0xE3;
            }
            secondaryOAM[oamaddr++] = value;
        }
        break;
    case 5:
        if (w) {
            // second write
            t = (t & ~(0b1110011'11100000))
                | ((u16)value & (0b11111 << 3)) << 2
                | ((u16)value & 0b111) << 12;
        } else {
            // first write
            t = (t & ~(0b11111)) | value >> 3;
            x = value & 0b111;
        }
        w = !w;
        break;
    case 6:
        if (w) {
            // second write
            //t: ....... ABCDEFGH <- d: ABCDEFGH
            //v: <...all bits...> <- t: <...all bits...>
            //w:                  <- 0
            t = (t & ~0xff) | value;
            v = t;
        } else {
            //first write
            //t: .CDEFGH ........ <- d: ..CDEFGH
            //       <unused>     <- d: AB......
            //t: Z...... ........ <- 0 (bit Z is cleared)
            //w:                  <- 1
            t = (t & ~(0b111111 << 8)) | (value & 0b111111) << 8;
            t &= (1 << 14) - 1; // clear the MSB
        }
        w = !w;
        break;
    case 7:
        if ((v & 0x3FFF) >= 0x3F00) {
            u16 address = v;
            address &= 0x1F;
            value &= 0x3F;
            palette[address] = value;
            if ((address & 0b11) == 0) {
                palette[address ^ 0x10] = value;
            }
        } else {
            if (scanline >= 240 || !RenderingEnabled()) {
                bus.WriteVRAM8(v, value);
                // TODO - PPU state updates happen a few cycles later
                // but we should be fine to just update immediately
                UpdateVideoRamAddr();
//                _mapper->WriteVram(_ppuBusAddress & 0x3FFF, value);
            } else {
                //During rendering, the value written is ignored, and instead the address' LSB is used (not confirmed, based on Visual NES)
//                _mapper->WriteVram(_ppuBusAddress & 0x3FFF, _ppuBusAddress & 0xFF);
            }
        }
        break;
    default:
        break;
    }
    latch = value;
}

void PPU::OAMDMA() {

}

void PPU::CatchUp() {
    // Before running apply any cached updates
    while (!bus.ppu_register_cache.empty()) {
        auto& item = bus.ppu_register_cache.front();
        if (item.is_write) {
            WriteRegister(item.addr, item.value);
        } else {
            // This is a write cache, so right now we don't add reads.
            // Later if we start scanning for reading $2002 for clearing
            // the write toggle (IE: the value read from ppustatus is unused)
            // then we could cache that read.
            // ReadRegister(item.addr);
        }
        bus.ppu_register_cache.pop();
    }
    s64 cycles_to_run = timing.cycle_count - current_cycle;
    while (cycles_to_run > 0) {
        // We can't use the fast scanline impl if we aren't on cycle 0
        if (cycles_to_run > CYCLES_PER_SCANLINE && cycle == 0) {
            RunFastScanline();
        } else {
            Tick();
        }
        if (cycle == 341) {
            cycle = 0;
            scanline++;
        }
        if (scanline == 262) {
            scanline = 0;
        }
        cycles_to_run = timing.cycle_count - current_cycle;
    }
}

void PPU::Tick() {
    if (scanline == 241 && cycle == 1) {
        status.vblank = 1;
    }

    // Cycle 0
    // Idle

    // Cycle 1 - 256

    // Cycle 257-320

    // Cycle 321-336

    // Cycle 337-340
    current_cycle += 4;
    cycle += 1;
}

// High level scanline render implementation for speed
void PPU::RunFastScanline() {
    if (scanline == 241) {
        status.vblank = 1;
    }
    if (scanline == 0) {
        status.vblank = 0;
    }

    OAMEvaluation();

    // Shortcut: Copy the entire horizontal strip of tile ids from the nametable
    // Load the nametable strip for this address, including across the mirror
    std::array<u8, 0x40> nmt_strip{};
    std::array<u8, 0x10> atr_strip{};
    // Intentionally strip the bottom 5 bits since we want the start of the row
    u16 tile_strip_addr = (v & 0x0F20);
    u16 tile_strip_attr = 0x3C0 | ((v >> 4) & 0x38);
    u8* left_nmt = bus.DirectVRAMPageAccess(0x2000 | tile_strip_addr);
    u8* right_nmt = bus.DirectVRAMPageAccess(0x2400 | tile_strip_addr);
    for (int i = 0; i < 0x20; i++) {
        nmt_strip[i] = left_nmt[i];
        nmt_strip[i + 0x20] = right_nmt[i];
    }
    for (int i = 0; i < 0x8; i++) {
        atr_strip[i] = left_nmt[tile_strip_attr | i];
        atr_strip[i + 0x08] = right_nmt[tile_strip_attr | i];
    }

    

    current_cycle += CYCLES_PER_SCANLINE;
    scanline++;

}

void PPU::OAMEvaluation() {
    if (!mask.sp_enable) {
        return;
    }



}


void PPU::UpdateVideoRamAddr() {
    if(scanline >= 240 || !RenderingEnabled()) {
        v = (v + (ctrl.vertical_write ? 32 : 1)) & 0x7FFF;
        //Trigger memory read when setting the vram address - needed by MMC3 IRQ counter
        //"Should be clocked when A12 changes to 1 via $2007 read/write"
        bus.vram_addr = (v & 0x3FFF);
    } else {
        //"During rendering (on the pre-render line and the visible lines 0-239, provided either background or sprite rendering is enabled), "
        //it will update v in an odd way, triggering a coarse X vertical_write and a Y vertical_write simultaneously"
        IncrementHScroll();
        IncrementVScroll();
    }
}

// http://wiki.nesdev.com/w/index.php/The_skinny_on_NES_scrolling#Wrapping_around
inline void PPU::IncrementHScroll() {
    u16 addr = v;
    if ((addr & 0b11111) == 31) {
        //When the value is 31, wrap around to 0 and switch nametable
        addr = (addr & ~0b11111) ^ 0x0400;
    } else {
        addr++;
    }
    v = addr;
}

// http://wiki.nesdev.com/w/index.php/The_skinny_on_NES_scrolling#Wrapping_around
void PPU::IncrementVScroll() {
    uint16_t addr = v;
    if ((addr & 0x7000) != 0x7000) {
        // if fine Y < 7, increment fine Y
        addr += 0x1000;
    } else {
        // fine Y = 0
        addr &= ~0x7000;
        // let y = coarse Y
        u8 y = (addr & 0x03E0) >> 5;
        if (y == 29) {
            // coarse Y = 0
            y = 0;
            // switch vertical nametable
            addr ^= 0x0800;
        } else if (y == 31) {
            // coarse Y = 0, nametable not switched
            y = 0;
        } else {
            // increment coarse Y
            y++;
        }
        // put coarse Y back into v
        addr = (addr & ~0x03E0) | (y << 5);
    }
    v = addr;
}
