

#ifndef BRUTENES_BUS_H
#define BRUTENES_BUS_H

#include <array>
#include <vector>

#include "common.h"
#include "virtmem.h"

class Bus {
public:
    Bus(std::span<u8> prg, std::span<u8> chr);

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

    inline u8 ReadVRAM8(u16 addr) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        return fakemmu.ppumem[bank][offset];
    }
    inline u16 ReadVRAM16(u16 addr) {
        return Read8(addr + 1) << 8 | Read8(addr);
    }
    inline void WriteVRAM8(u16 addr, u8 value) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        fakemmu.ppumem[bank][offset] = value;
    }

    // Latest vram address from the PPU increment
    u16 vram_addr{};

private:

//    void InitHostMMU(std::span<u8> prg, std::span<u8> chr);
    void InitFakeMMU(std::span<u8> prg, std::span<u8> chr);

    HostVirtualMemory hostmmu{};
    FakeVirtualMemory fakemmu{};

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
