

#ifndef BRUTENES_BUS_H
#define BRUTENES_BUS_H

#include <array>
#include <queue>
#include <vector>

#include "common.h"
#include "controller.h"
#include "ppu.h"
#include "virtmem.h"

class INES;

class Bus {
public:

    struct RegisterCacheEntry {
        u16 addr;
        u8 value;
        bool is_write;
    };

    Bus(const INES& header, std::span<u8> prg, std::span<u8> chr, PPU& ppu,
        Controller& controller1, Controller& controller2);

    // TODO better open bus handling
    inline u8 OpenBus() const {
        return 0;
    }

//    inline u8 Read8(u16 addr) { return memory.Span()[addr * 4]; }
//    inline u16 Read16(u16 addr) { return (memory.Span()[(addr + 1) * 4] << 8) | memory.Span()[addr * 4]; }
//
//    inline void Write8(u16 addr, u8 value) { memory.Span()[addr * 4] = value; }

// I'll revisit using the HOST MMU later, but for now, we can use poormans

    inline u8 Read8(u16 addr) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        return fakemmu.cpumem[bank][offset];
    }
    inline u16 Read16(u16 addr) {
        return Read8(addr + 1) << 8 | Read8(addr);
    }
    inline void Write8(u16 addr, u8 value) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        fakemmu.cpumem[bank][offset] = value;
    }

    constexpr inline bool CheckedRead8(u16 addr, u8& out) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        if ((fakemmu.cputag[bank] & FakeVirtualMemory::Tag::MMIO) != 0) {
            return false;
        }
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        out = fakemmu.cpumem[bank][offset];
        return true;
    }

    constexpr inline bool CheckedRead16(u16 addr, u16& out) {
        u8 lo, hi;
        if (!CheckedRead8(addr, lo) || !CheckedRead8(addr, hi)) {
            return false;
        }
        out = (lo) | ((u16)hi << 8);
        return true;
    }

    constexpr inline bool CheckedWrite8(u16 addr, u8 value) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto tag = fakemmu.cputag[bank];
        if ((tag & FakeVirtualMemory::Tag::MMIO) != 0) {
            return false;
        }
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        // Dont write if not mapped as writable, but return true since it is mapped
        if ((tag & FakeVirtualMemory::Tag::Write) != 0)
            fakemmu.cpumem[bank][offset] = value;
        return true;
    }

    inline u8 ReadVRAM8(u16 addr) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        return fakemmu.ppumem[bank][offset];
    }

    inline u8* DirectVRAMPageAccess(u16 addr) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        return fakemmu.ppumem[bank];
    }

    inline u8* PixelCacheLookup(u16 addr, u8 fine_y) {
        constexpr int TILE_STRIDE = 64;
        constexpr int PIXEL_STRIDE = 8;
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto tile_addr = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        int x = ((tile_addr >> 4) & 0xf) * TILE_STRIDE;
        int y = ((tile_addr >> 8) & 0xf) * TILE_STRIDE * 16 + fine_y * PIXEL_STRIDE;
        return &fakemmu.chr_pixel_map[bank][x + y];
    }

//    inline u16 ReadVRAM16(u16 addr) {
//        return Read8(addr + 1) << 8 | Read8(addr);
//    }
    inline void WriteVRAM8(u16 addr, u8 value) {
//        addr &= 0x3fff;
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        fakemmu.ppumem[bank][offset] = value;
    }

    inline u8 ReadPPURegister(u16 addr) {
        return ppu.ReadRegister(addr);
    }
    inline void WritePPURegister(u16 addr, u8 value, bool use_ppu_cache) {
        if (use_ppu_cache) {
            ppu_register_cache.emplace(RegisterCacheEntry{ addr, value, true });
        } else {
            ppu.WriteRegister(addr, value);
        }
    }

    inline u8 ReadAPURegister(u16 addr) {
        if (addr == 0x4016 || addr == 0x4017) {
            if (addr & 1) {
                return controller2.ReadRegister(addr);
            } else {
                return controller1.ReadRegister(addr);
            }
            return 0;
        }
        // Unimplemented $4015 for now
        return OpenBus();
    }
    inline void WriteAPURegister(u16 addr, u8 value, bool cache) {
        if (addr == 0x4016 || addr == 0x4017) {
            if (addr & 1) {
                controller2.WriteRegister(addr, value);
            } else {
                controller1.WriteRegister(addr, value);
            }
        }
        // Unimplemented APU writes
    }

    // Latest vram address from the PPU increment
    u16 vram_addr{};

    std::queue<RegisterCacheEntry> ppu_register_cache{};

private:

//    void InitHostMMU(std::span<u8> prg, std::span<u8> chr);
//    void InitFakeMMU(std::span<u8> prg, std::span<u8> chr);

    //HostVirtualMemory hostmmu{};
    FakeVirtualMemory fakemmu{};
    PPU& ppu;
    Controller& controller1;
    Controller& controller2;

    // CPU memory cpumem backing
//    std::span<u8> ciram;
//    std::span<u8> mmio;
//    std::array<std::span<u8>, 10> prgmap{};
//    // PRG ROM banks
//    std::vector<std::span<u8>> prgbank{};
//    // CHR ROM banks
//    std::vector<std::span<u8>> chrbank{};
//    // PPU memory cpumem backing
//    std::array<std::span<u8>, 8> chrmap{};
//    std::array<std::span<u8>, 4> nmtmap{};
//    std::span<u8> palette;
};


#endif //BRUTENES_BUS_H
