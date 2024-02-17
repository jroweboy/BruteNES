

#ifndef BRUTENES_BUS_H
#define BRUTENES_BUS_H

#include <array>
#include <vector>

#include "common.h"
#include "virtmem.h"

class Bus {
public:
    Bus(std::span<u8> prg, std::span<u8> chr);

//    inline u8 Read8(u16 addr) { return memory.Span()[addr * 4]; }
//    inline u16 Read16(u16 addr) { return (memory.Span()[(addr + 1) * 4] << 8) | memory.Span()[addr * 4]; }
//
//    inline void Write8(u16 addr, u8 value) { memory.Span()[addr * 4] = value; }

// I'll revisit using the HOST MMU later, but for now, we can use poormans

    inline u8 Read8(u16 addr) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        return fakemmu.map[bank][offset];
    }
    inline u16 Read16(u16 addr) {
        return Read8(addr + 1) << 8 | Read8(addr);
    }
    inline void Write8(u16 addr, u8 value) {
        const auto bank = addr / FakeVirtualMemory::BANK_WINDOW;
        const auto offset = addr & (FakeVirtualMemory::BANK_WINDOW-1);
        fakemmu.map[bank][offset] = value;
    }


private:

    void InitHostMMU(std::span<u8> prg, std::span<u8> chr);
    void InitFakeMMU(std::span<u8> prg, std::span<u8> chr);

    HostVirtualMemory hostmmu{};
    FakeVirtualMemory fakemmu{};

    // CPU memory map backing
    std::span<u8> ciram;
    std::span<u8> mmio;
    std::array<std::span<u8>, 10> prgmap{};
    // PRG ROM banks
    std::vector<std::span<u8>> prgbank{};
    // CHR ROM banks
    std::vector<std::span<u8>> chrbank{};
    // PPU memory map backing
    std::array<std::span<u8>, 8> chrmap{};
    std::array<std::span<u8>, 4> nmtmap{};
    std::span<u8> palette;
};


#endif //BRUTENES_BUS_H
