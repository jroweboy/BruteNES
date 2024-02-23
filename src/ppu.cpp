
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

        IncrementV();
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
        oam_addr = value;
        break;
    case 4:
        if (scanline < 240 && RenderingEnabled()) {
            //"Writes to OAMDATA during rendering (on the pre-render line and the visible lines 0-239, provided either sprite or background rendering is enabled) do not modify values in OAM,
            //but do perform a glitchy vertical_write of OAMADDR, bumping only the high 6 bits"
            oam_addr += 4;
        } else {
            //"The three unimplemented bits of each sprite's byte 2 do not exist in the PPU and always read back as 0 on PPU revisions that allow reading PPU OAM through OAMDATA ($2004)"
            if((oam_addr & 0x03) == 0x02) {
                value &= 0xE3;
            }
            OAM[oam_addr++] = value;
        }
        break;
    case 5:
        if (!w) {
            // first write
            //t: ....... ...ABCDE <- d: ABCDE...
            //x:              FGH <- d: .....FGH
            //w:                  <- 1
            t = (t & ~(0b11111)) | value >> 3;
            x = value & 0b111;
        } else {
            // second write
            //t: FGH..AB CDE..... <- d: ABCDEFGH
            //w:                  <- 0
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
            IncrementV();
        } else {
            if (scanline >= 240 || !RenderingEnabled()) {
                bus.WriteVRAM8(v, value);
                // TODO - PPU state updates happen a few cycles later
                // but we should be fine to just update immediately
                IncrementV();
//                _mapper->WriteVram(_ppuBusAddress & 0x3FFF, value);
            } else {
                bus.WriteVRAM8(v & 0x3FFF, v & 0xff);
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
        if (scanline > SCANLINE_PRERENDER) {
            scanline = 0;
        }
        cycles_to_run = timing.cycle_count - current_cycle;
    }
}

void PPU::Tick() {
    if (scanline == SCANLINE_VBLANK_START && cycle == 1) {
        status.vblank = 1;
    }
    if (scanline == SCANLINE_FRAME_START && cycle == 1) {
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
        if (cycle == 257) {
            CopyHScrollToV();
        }
        if (scanline == SCANLINE_PRERENDER && cycle == 280) {
            v = t;
        }
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
    if (scanline == SCANLINE_VBLANK_START) {
        status.vblank = 1;
    }
    if (scanline == SCANLINE_FRAME_START) {
        status.vblank = 0;
        status.sp_zero = 0;
    }

    if (RenderingEnabled()) {
        if (scanline == SCANLINE_PRERENDER) {
            v = t;
        }

        if (scanline < SCANLINE_VBLANK_START) {
            FastScanlineRender();
            // OAM evaluation happens for the scanline *after* so read the next scanline's data in now
            FastOAMEvaluation();
        }
    }

    current_cycle += CYCLES_PER_SCANLINE;
    scanline++;
}

void PPU::FastOAMEvaluation() {
    /*
During all visible scanlines, the PPU scans through OAM to determine which sprites to render on the next scanline. Sprites found to be within range are copied into the secondary OAM, which is then used to initialize eight internal sprite output units.

OAM[n][m] below refers to the byte at offset 4*n + m within OAM, i.e. OAM byte m (0-3) of sprite n (0-63).

During each pixel clock (341 total per scanline), the PPU accesses OAM in the following pattern:

    Cycles 1-64: Secondary OAM (32-byte buffer for current sprites on scanline) is initialized to $FF - attempting to read $2004 will return $FF. Internally, the clear operation is implemented by reading from the OAM and writing into the secondary OAM as usual, only a signal is active that makes the read always return $FF.
    Cycles 65-256: Sprite evaluation
        On odd cycles, data is read from (primary) OAM
        On even cycles, data is written to secondary OAM (unless secondary OAM is full, in which case it will read the value in secondary OAM instead)
        1. Starting at n = 0, read a sprite's Y-coordinate (OAM[n][0], copying it to the next open slot in secondary OAM (unless 8 sprites have been found, in which case the write is ignored).
            1a. If Y-coordinate is in range, copy remaining bytes of sprite data (OAM[n][1] thru OAM[n][3]) into secondary OAM.
        2. Increment n
            2a. If n has overflowed back to zero (all 64 sprites evaluated), go to 4
            2b. If less than 8 sprites have been found, go to 1
            2c. If exactly 8 sprites have been found, disable writes to secondary OAM because it is full. This causes sprites in back to drop out.
        3. Starting at m = 0, evaluate OAM[n][m] as a Y-coordinate.
            3a. If the value is in range, set the sprite overflow flag in $2002 and read the next 3 entries of OAM (incrementing 'm' after each byte and incrementing 'n' when 'm' overflows); if m = 3, increment n
            3b. If the value is not in range, increment n and m (without carry). If n overflows to 0, go to 4; otherwise go to 3
                The m increment is a hardware bug - if only n was incremented, the overflow flag would be set whenever more than 8 sprites were present on the same scanline, as expected.
        4. Attempt (and fail) to copy OAM[n][0] into the next free slot in secondary OAM, and increment n (repeat until HBLANK is reached)
    Cycles 257-320: Sprite fetches (8 sprites total, 8 cycles per sprite)
        1-4: Read the Y-coordinate, tile number, attributes, and X-coordinate of the selected sprite from secondary OAM
        5-8: Read the X-coordinate of the selected sprite from secondary OAM 4 times (while the PPU fetches the sprite tile data)
        For the first empty sprite slot, this will consist of sprite #63's Y-coordinate followed by 3 $FF bytes; for subsequent empty sprite slots, this will be four $FF bytes
    Cycles 321-340+0: Background render pipeline initialization
        Read the first byte in secondary OAM (while the PPU fetches the first two background tiles for the next scanline)
     */
    constexpr auto SpriteInRange = [](auto line, auto y, auto sprite_size) {
        return line >= y && line < y + sprite_size;
    };
    spriteZeroFound = false;
    const u8 sprite_size = ctrl.sp_size ? 16 : 8;
    std::memset(scanline_sp_attribute.data(), 0x00, scanline_sp_attribute.size() * sizeof (u8));
    std::memset(scanline_sp_palette_buffer.data(), 0x00, scanline_sp_palette_buffer.size() * sizeof (u8));
    // Cycles 1-64: Secondary OAM (32-byte buffer for current sprites on scanline) is initialized to $FF
    // - attempting to read $2004 will return $FF. Internally, the clear operation is implemented by reading
    // from the OAM and writing into the secondary OAM as usual, only a signal is active that makes the read always return $FF.
    std::memset(secondaryOAM.data(), 0xff, secondaryOAM.size() * sizeof (u8));

    u8 written = 0;
    u16 n = 0;
    while (n < 256) {
        // 1. Starting at n = 0, read a sprite's Y-coordinate (OAM[n][0], copying it to the next open slot in secondary
        // OAM (unless 8 sprites have been found, in which case the write is ignored).
        //   1a. If Y-coordinate is in range, copy remaining bytes of sprite data (OAM[n][1] thru OAM[n][3]) into secondary OAM.
        secondaryOAM[written * 4] = OAM[n];
        if (SpriteInRange(scanline, OAM[n], sprite_size)) {
            if (n == 0) {
                spriteZeroFound = true;
            }
            secondaryOAM[written * 4 + 1] = OAM[n + 1];
            secondaryOAM[written * 4 + 2] = OAM[n + 2];
            secondaryOAM[written * 4 + 3] = OAM[n + 3];
            written++;
        }
        // 2. Increment n
        //   2a. If n has overflowed back to zero (all 64 sprites evaluated), go to 4
        //   2b. If less than 8 sprites have been found, go to 1
        //   2c. If exactly 8 sprites have been found, disable writes to secondary OAM because it is full. This causes sprites in back to drop out.
        n += 4;
        if (written == 8) {
            break;
        }
    }

    // Start checking for sprite overflow
    // 3. Starting at m = 0, evaluate OAM[n][m] as a Y-coordinate.
    if (n < 256) {
        int m = 0;
        while (n < 256) {
            if (SpriteInRange(scanline, OAM[n + m], sprite_size)) {
                //  3a. If the value is in range, set the sprite overflow flag in $2002 and read the next 3 entries of OAM
                //    (incrementing 'm' after each byte and incrementing 'n' when 'm' overflows); if m = 3, increment n
                status.sp_overflow = 1;
                n += 4;
            } else {
                //  3b. If the value is not in range, increment n and m (without carry). If n overflows to 0, go to 4;
                //    otherwise go to 3
                //    The m increment is a hardware bug - if only n was incremented, the overflow flag would be set whenever
                //    more than 8 sprites were present on the same scanline, as expected.
                n += 4;
                m = (m + 1) % 4;
            }
        }
    }
    // 4. Attempt (and fail) to copy OAM[n][0] into the next free slot in secondary OAM, and increment n (repeat until HBLANK is reached)
    // soooo... do nothing?

    // Cycles 257-320: Sprite fetches (8 sprites total, 8 cycles per sprite)
    //   1-4: Read the Y-coordinate, tile number, attributes, and X-coordinate of the selected sprite from secondary OAM
    //   5-8: Read the X-coordinate of the selected sprite from secondary OAM 4 times (while the PPU fetches the sprite tile data)
    //   For the first empty sprite slot, this will consist of sprite #63's Y-coordinate followed by 3 $FF bytes; for subsequent empty sprite slots, this will be four $FF bytes

    // Copy the attribute flags into the attribute buffer and then also
    for (int i = 0; i < written; i++) {
        // Get the X coord of the sprite
        u8 spr_y = secondaryOAM[i * 4 + 0];
        u8 spr_tile = secondaryOAM[i * 4 + 1];
        u8 spr_attr = secondaryOAM[i * 4 + 2];
        u8 spr_x = secondaryOAM[i * 4 + 3];
        u16 chr_tile_addr = (ctrl.sp_pattern << 12) | spr_tile * 16;
        u8 fine_y = scanline - spr_y;
        if ((spr_attr & (1 << 7)) != 0) {
            // Vertical flip is set so invert spr_y;
            fine_y = (spr_y + sprite_size) - scanline;
        }
        u8 fine_x = ((spr_attr & (1 << 6)) != 0) ? 7 : 0;
        u8 fine_x_increment = ((spr_attr & (1 << 6)) != 0) ? (u8)-1 : 1;
        u8* pixel_cache = bus.PixelCacheLookup(chr_tile_addr, fine_y);
        // for each pixel in this sprite, check if there is already a non transparent pixel and cover it
        for (int j = spr_x; j < spr_x + 8; j++) {
            if (j > 256) break;
            if (scanline_sp_palette_buffer[j] == 0) {
                scanline_sp_palette_buffer[j] = pixel_cache[fine_x];

                scanline_sp_attribute[j] = spr_attr;
                if (i == 0 && spriteZeroFound) {
                    // This always reads back zero, so we are allowed to overwrite this but only in internal buffers
                    scanline_sp_attribute[j] |= SPRITE_ZERO_TAG;
                }
            }
            // If horizontally flipped, then read it from right to left instead
            fine_x += fine_x_increment;
        }
    }
}


void PPU::IncrementV() {
    if(scanline >= SCANLINE_VBLANK_START || !RenderingEnabled()) {
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


void PPU::FastScanlineRender() {
    // Shortcut: Copy the entire horizontal strip of tile ids from the nametable
    // Load the nametable strip for this address, including across the mirror
    std::array<u8, 0x40> nmt_strip{};
    std::array<u8, 0x40> atr_strip{};
    // Intentionally strip the bottom 5 bits since we want the start of the row
    u16 course_y = (v & 0b11'11100000);
    // Now that we have our line of tiles to read from, decode the pixels
    // into the palette data
    u8 fine_x = x;
    // Combine the low bit of the nametable with the coarse x
    u8 coarse_x = (v & (0x20-1)) | ((v & 0b100'00000000) >> 5);
    u8 fine_y = v >> 12;

    u16 tile_strip_addr = 0x2000 | course_y;
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

    for (int i = 0; i < 256; i++) {
        // What really happens in the NES PPU is conceptually more like this:
        //    For each pixel in the background buffer, the corresponding sprite pixel replaces it only if the
        //    sprite pixel is opaque and front priority or if the background pixel is transparent.
        u8 spr_attr = scanline_sp_attribute[i];
        u8 spr_palette = scanline_sp_palette_buffer[i];
        if (spr_palette != 0 && spr_attr != 0) {
            const bool isSpriteZeroPixel = spr_attr & SPRITE_ZERO_TAG;
            if (isSpriteZeroPixel) {
                // use some number larger than the total number of cycles to indicate that sprites aren't enabled
                const int _minimumDrawSpriteStandardCycle = mask.sp_enable ? (mask.sp_left ? 0 : 8) : 300;
                if (scanline_bg_pixel_buffer[i] != 0 && i != 255 && mask.bg_enable && !status.sp_zero && i > _minimumDrawSpriteStandardCycle) {
                    status.sp_zero = 1;
                }
            }
        }
        // Sprite priority
        const bool bg_priority = (spr_attr & (1 << 5)) != 0;
        if ((bg_priority && spr_palette != 0) || spr_palette == 0) {
            rendering_to[(scanline * 256) + i] = scanline_bg_pixel_buffer[i];
        } else {
            u8* pal = &palette[(spr_attr&0b11) + 0x10];
            rendering_to[(scanline * 256) + i] = pal[scanline_sp_palette_buffer[i]];
        }
    }

    IncrementVScroll();
    CopyHScrollToV();
}