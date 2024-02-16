

#ifndef BRUTENES_BUS_H
#define BRUTENES_BUS_H

#include <array>
#include <vector>

#include "common.h"
#include "virtmem.h"

class Bus {
public:
    Bus(std::span<u8> prg, std::span<u8> chr);

    inline u8 Read() { return 0; }

private:

    VirtualMemory memory;
    // CPU memory map backing
    std::span<u8> ciram;
    std::span<u8> mmio;
    std::array<std::span<u8>, 10> prgmap;
    // PRG ROM banks
    std::vector<std::span<u8>> prgbank;
    // CHR ROM banks
    std::vector<std::span<u8>> chrbank;
    // PPU memory map backing
    std::array<std::span<u8>, 8> chrmap;
    std::array<std::span<u8>, 4> nmtmap;
    std::span<u8> palette;
};


#endif //BRUTENES_BUS_H
