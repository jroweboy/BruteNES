
#include <range/v3/view.hpp>

#include "bus.h"
#include "ines.h"


Bus::Bus(const INES& header, std::span<u8> prg, std::span<u8> chr, PPU& ppu) : ppu(ppu) {
    fakemmu.InitCPUMap(prg);
    fakemmu.InitPPUMap(chr, header);
//    InitFakeMMU(prg, chr);
//    InitHostMMU(prg, chr);
}

//void Bus::InitHostMMU(std::span<u8> prg, std::span<u8> chr) {
//    // We can only swap pages on 4kb boundaries, and we need to have
//    // at least 1kb banking∂, so we multiply all addresses
//    // by 4 to align the bank size with the host MMU page size
//    constexpr auto OFFSET = 4;
//    constexpr auto MAX_PRG_SIZE = 4 * 1024 * 1024 * OFFSET;
//    constexpr auto MAX_CHR_SIZE = 4 * 1024 * 1024 * OFFSET;
//    constexpr auto CHR_BANK_SIZE = 0x400 * OFFSET;
//    constexpr auto PRG_BANK_SIZE = 0x1000 * OFFSET;
//    constexpr auto logical_address_space = 0x10000 * OFFSET;
//    constexpr auto phyiscal_address_space = 0x10000 * OFFSET;
//    constexpr auto PHYS_OFFSET = logical_address_space;
//    constexpr auto PRG_OFFSET = logical_address_space + phyiscal_address_space;
//    constexpr auto CHR_OFFSET = PRG_OFFSET + MAX_PRG_SIZE;
//    hostmmu.Reserve(logical_address_space + phyiscal_address_space + MAX_PRG_SIZE + MAX_CHR_SIZE);
//
//    // Alloc the view for the CIRAM and cpumem it to the mirrors
//    ciram = hostmmu.CreateView(PHYS_OFFSET, 0x800 * OFFSET);
//    hostmmu.MapView(ciram, 0);
//    hostmmu.MapView(ciram, 0x800 * OFFSET);
//    hostmmu.MapView(ciram, 0x800 * 2 * OFFSET);
//    hostmmu.MapView(ciram, 0x800 * 3 * OFFSET);
//
//    mmio = hostmmu.CreateView(PHYS_OFFSET + 0x2000 * OFFSET, 0x4000 * OFFSET);
//    hostmmu.MapView(mmio, 0x4000 * OFFSET);
//
//    u32 i = 0;
//    for (const auto& slice : prg | ranges::views::chunk(0x1000)) {
//        auto mem = hostmmu.CreateView(PRG_OFFSET + (PRG_BANK_SIZE * (i++)) , PRG_BANK_SIZE);
//        // copy the prg rom data into the memory array
//        u32 j = 0;
//        for (const auto& b : slice) {
//            mem[j++ * 4] = b;
//        }
//        prgbank.push_back(mem);
//    }
//
//    // Map all banks to our virtual address space starting from the fixed bank
//    i = 10;
//    for (auto& prgview : prgbank | ranges::views::reverse) {
//        i--;
//        prgmap[i] = hostmmu.MapView(prgview, 0x6000 * OFFSET + (PRG_BANK_SIZE * i));
//    }
//
//    // Create CHR banks from the raw CHR copying over the data
//    i = 0;
//    for (const auto& slice : chr | ranges::views::chunk(0x400)) {
//        auto mem = hostmmu.CreateView(CHR_OFFSET + (CHR_BANK_SIZE * (i++)), CHR_BANK_SIZE);
//        // copy the chr rom data into the memory array
//        u32 j = 0;
//        for (const auto& b : slice) {
//            mem[j++ * 4] = b;
//        }
//        chrbank.push_back(mem);
//    }
////    i = 0;
////    for (auto& chrview : chrbank) {
////        chrmap[i] = hostmmu.MapView(chrview, 0x0000 * OFFSET + (CHR_BANK_SIZE * i));
////        i++;
////    }
//
//}

