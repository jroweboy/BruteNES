

#ifndef BRUTENES_PPU_H
#define BRUTENES_PPU_H

#include <mutex>

#include "common.h"
#include "scheduler.h"

class Bus;
class Scheduler;

class PPU {
public:
    constexpr static auto CYCLES_PER_SCANLINE = 341 * Scheduler::NTSC_PPU_CLOCK_DIVIDER;

    explicit PPU(Bus& bus, Scheduler& timing) : bus(bus), timing(timing) {}

    u8 ReadRegister(u16 addr);
    void WriteRegister(u16 addr, u8 value);

    void CatchUp();
    void Tick();

    bool even_frame{};

/*
7  bit  0
---- ----
VPHB SINN
|||| ||||
|||| ||++- Base nametable address
|||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
|||| |+--- VRAM address vertical_write per CPU read/write of PPUDATA
|||| |     (0: add 1, going across; 1: add 32, going down)
|||| +---- Sprite pattern table address for 8x8 sprites
||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
|||+------ Background pattern table address (0: $0000; 1: $1000)
||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
|+-------- PPU master/slave select
|          (0: read backdrop from EXT pins; 1: output color on EXT pins)
+--------- Generate an NMI at the start of the
           vertical blanking interval (0: off; 1: on)
*/
    union PPUCTRL {
        u8 raw;
        struct {
            u8 nametable : 2;
            u8 vertical_write : 1;
            u8 sp_pattern : 1;
            u8 bg_pattern : 1;
            u8 sp_size : 1;
            u8 select : 1;
            u8 nmi : 1;
        };
    };

/*
7  bit  0
---- ----
BGRs bMmG
|||| ||||
|||| |||+- Greyscale (0: normal color, 1: produce a greyscale display)
|||| ||+-- 1: Show background in leftmost 8 pixels of screen, 0: Hide
|||| |+--- 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
|||| +---- 1: Show background
|||+------ 1: Show sprites
||+------- Emphasize red (green on PAL/Dendy)
|+-------- Emphasize green (red on PAL/Dendy)
+--------- Emphasize blue
 */
    union PPUMASK {
        u8 raw;
        struct {
            u8 greyscale : 1;
            u8 bg_left : 1;
            u8 sp_left : 1;
            u8 bg_enable : 1;
            u8 sp_enable : 1;
            u8 emp_red : 1;
            u8 emp_green : 1;
            u8 emp_blue : 1;
        };
    };

/*
7  bit  0
---- ----
VSO. ....
|||| ||||
|||+-++++- PPU open bus. Returns stale PPU bus contents.
||+------- Sprite overflow. The intent was for this flag to be set
||         whenever more than eight sprites appear on a scanline, but a
||         hardware bug causes the actual behavior to be more complicated
||         and generate false positives as well as false negatives; see
||         PPU sprite evaluation. This flag is set during sprite
||         evaluation and cleared at dot 1 (the second dot) of the
||         pre-render line.
|+-------- Sprite 0 Hit.  Set when a nonzero pixel of sprite 0 overlaps
|          a nonzero background pixel; cleared at dot 1 of the pre-render
|          line.  Used for raster timing.
+--------- Vertical blank has started (0: not in vblank; 1: in vblank).
           Set at dot 1 of line 241 (the line *after* the post-render
           line); cleared after reading $2002 and at dot 1 of the
           pre-render line.
 */
    union PPUSTATUS {
        u8 raw;
        struct {
            u8 openbus : 5;
            u8 sp_overflow : 1;
            u8 sp_zero : 1;
            u8 vblank : 1;
        };
    };

    PPUCTRL ctrl{};
    PPUMASK mask{};
    PPUSTATUS status{};

    u8 oamaddr{};

    [[nodiscard]] inline bool RenderingEnabled() const {
        return mask.bg_enable || mask.sp_enable;
    }

    u16* LockBuffer() {
        std::unique_lock<std::mutex> lock(swap_mutex);
        std::swap(front_buffer, back_buffer);
        return front_buffer;
    }

    void SwapBuffer() {
        std::unique_lock<std::mutex> lock(swap_mutex);
        std::swap(back_buffer, rendering_to);
    }


private:

    bool w{}; // 1 bit - write toggle
    u16 v{}; // 15 bits - vram addr
    u16 t{}; // 15 bits - temporary vram addr
    u8 x{}; // 3 bits - fine_x scroll

    u8 latch{};

    void OAMDMA();
    void RunFastScanline();
    void OAMEvaluation();
    void UpdateVideoRamAddr();
    inline void IncrementHScroll();
    inline void IncrementVScroll();

    std::array<u8, 256> secondaryOAM{};
    std::array<u8, 0x20> palette{};

    Bus& bus;
    Scheduler& timing;
    u64 current_cycle{};

    // Internal scanline cycle count
    u16 scanline{};
    u16 cycle{};

    std::array<u16, 256 * 240> pixel_buffer1{};
    std::array<u16, 256 * 240> pixel_buffer2{};
    std::array<u16, 256 * 240> pixel_buffer3{};

    std::mutex swap_mutex{};
    u16* front_buffer = pixel_buffer1.data();
    u16* back_buffer = pixel_buffer2.data();
    u16* rendering_to = pixel_buffer3.data();
    std::array<u16*, 3> buffers = {pixel_buffer1.data(), pixel_buffer2.data(), pixel_buffer3.data()};
};


#endif //BRUTENES_PPU_H
