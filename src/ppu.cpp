
#include "bus.h"
#include "scheduler.h"
#include "ppu.h"

u8 PPU::ReadRegister(u16 addr) {
    switch (addr & 0b0000'0111) {
    case 2:
        w = false;
        status.openbus = latch;
        return status.raw;
    case 7: {
        u8 returnValue = read_buffer;
        read_buffer = bus.ReadVRAM8(v & (0x4000-1));

        UpdateVideoRamAddr();
        return returnValue;
    }
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
        if (!w) {
            // first write
            t = (t & ~(0b11111)) | value >> 3;
            x = value & 0b111;
        } else {
            // second write
            t = (t & ~(0b1110011'11100000))
                | ((u16)value & (0b11111 << 3)) << 2
                | ((u16)value & 0b111) << 12;
        }
        w = !w;
        break;
    case 6:
        if (!w) {
            //first write
            //t: .CDEFGH ........ <- d: ..CDEFGH
            //       <unused>     <- d: AB......
            //t: Z...... ........ <- 0 (bit Z is cleared)
            //w:                  <- 1
            t = (t & ~(0b111111 << 8)) | (value & 0b111111) << 8;
            t &= (1 << 14) - 1; // clear the MSB
        } else {
            // second write
            //t: ....... ABCDEFGH <- d: ABCDEFGH
            //v: <...all bits...> <- t: <...all bits...>
            //w:                  <- 0
            t = (t & ~0xff) | value;
            v = t;
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
            UpdateVideoRamAddr();
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
    if (scanline == 240 && cycle == 1) {
        status.vblank = 1;
    }
    if (scanline == 0) {
        status.vblank = 0;
        status.sp_zero = 0;
    }
    // At dot 256 of each scanline
    //
    // If rendering is enabled, the PPU increments the vertical position in v.
    // The effective Y scroll coordinate is incremented, which is a complex
    // operation that will correctly skip the attribute table memory regions,
    // and wrap to the next nametable appropriately.
    if (RenderingEnabled()) {
        if (cycle == 256) {
            IncrementVScroll();
        }
        if ( cycle == 257) {
            CopyHScrollToV();
        }
        if (scanline == 261 && cycle == 280) {
            v = t;
        }
    }

    // HACK: set sprite zero on scanline 26 for now
    if (scanline == 28 && cycle == 100) {
        status.sp_zero = 1;
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
    if (scanline == 240) {
        status.vblank = 1;
    }
    if (scanline == 0) {
        status.vblank = 0;
        status.sp_zero = 0;
    }

    if (RenderingEnabled()) {
        // HACK: set sprite zero on scanline 26 for now
        if (scanline == 28) {
            status.sp_zero = 1;
        }
        if (scanline == 261) {
            v = t;
        }

        if (scanline < 240) {
            // TODO: actually do this
            OAMEvaluation();

            // Shortcut: Copy the entire horizontal strip of tile ids from the nametable
            // Load the nametable strip for this address, including across the mirror
            std::array<u8, 0x40> nmt_strip{};
            std::array<u8, 0x40> atr_strip{};
            // Intentionally strip the bottom 5 bits since we want the start of the row
            u16 tile_strip_addr = 0x2000 | (v & 0b11'11100000);
            u16 tile_strip_attr = 0x3C0 | ((v >> 4) & 0x38);
            u8 tile_attr_top_or_bottom = (v & 0x7);
            u8* left_nmt = bus.DirectVRAMPageAccess(tile_strip_addr);
            u16 right_nmt_addr = tile_strip_addr ^ (0x400);
            u8* right_nmt = bus.DirectVRAMPageAccess(right_nmt_addr);
            for (int i = 0; i < 0x20; i++) {
                u16 offset = (tile_strip_addr & 0x3ff) | i;
                nmt_strip[i] = left_nmt[offset];
                nmt_strip[i + 0x20] = right_nmt[offset];
                // Decode the attribute for this tile from the actual attribute byte
                u8 attr_shift = tile_attr_top_or_bottom | ((i % 4) & 0b10);
                atr_strip[i] = (left_nmt[tile_strip_attr | (i/2)] >> attr_shift) & 0b11;
                atr_strip[i + 0x20] = (right_nmt[tile_strip_attr | (i/2)] >> attr_shift) & 0b11;
            }

            // Now that we have our line of tiles to read from, decode the pixels
            // into the palette data
            u8 fine_x = x;
            // Combine the low bit of the nametable with the coarse x
            u8 coarse_x = v & (0x20-1) | ((v & 0b0100'00000000) >> 5);
            u8 fine_y = v >> 12;

            // Initialize the data for rendering this strip
            // Lookup the tile in the pixel cache
            u8 tile = nmt_strip[coarse_x];
            u8 tile_attribute = atr_strip[coarse_x];
            u8* tile_palette = &palette[tile_attribute * 4];
            u16 chr_tile_addr = (ctrl.bg_pattern << 12) | tile * 16;
            u8* pixel_cache = bus.PixelCacheLookup(chr_tile_addr, fine_y);

            // For each tile in the background
            for (int i = 0; i < 0x20; i++) {
                // for each pixel in the tile
                for (int j = 0; j < 8; j++) {
                    const auto p = pixel_cache[fine_x];
                    scanline_bg_pixel_buffer[i*8 + j] = p ? tile_palette[p] : palette[0];
                    fine_x++;
                    if (fine_x == 8) {
                        fine_x = 0;
                        coarse_x = (coarse_x + 1) % 0x40;
                        tile = nmt_strip[coarse_x];
                        tile_attribute = atr_strip[coarse_x];
                        tile_palette = &palette[tile_attribute * 4];
                        chr_tile_addr = (ctrl.bg_pattern << 12) | tile * 16;
                        pixel_cache = bus.PixelCacheLookup(chr_tile_addr, fine_y);
                    }
                }
            }

            // TODO mix scanline bg and sp buffers together
            std::memcpy(rendering_to + (scanline * 256),
                        scanline_bg_pixel_buffer.data(),
                        scanline_bg_pixel_buffer.size() * sizeof(u16));
        }

        IncrementVScroll();
        CopyHScrollToV();
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

void PPU::CopyHScrollToV() {
    // v: ....A.. ...BCDEF <- t: ....A.. ...BCDEF
    constexpr u16 copy_mask = 0b0000100'00011111;
    v = (v & ~(copy_mask)) | (t & copy_mask);
}
