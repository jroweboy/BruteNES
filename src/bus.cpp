
#include <range/v3/view.hpp>

#include "bus.h"

Bus::Bus(std::span<u8> prg, std::span<u8> chr) : memory() {

    // We can only swap pages on 4kb boundaries, and we need to have
    // at least 1kb banking∂, so we multiply all addresses
    // by 4 to align the bank size with the host MMU page size
    constexpr auto OFFSET = 4;
    constexpr auto MAX_PRG_SIZE = 4 * 1024 * 1024 * OFFSET;
    constexpr auto MAX_CHR_SIZE = 4 * 1024 * 1024 * OFFSET;
    constexpr auto logical_address_space = 0x10000 * OFFSET;
    constexpr auto PRG_OFFSET = logical_address_space;
    constexpr auto CHR_OFFSET = PRG_OFFSET + MAX_PRG_SIZE;
    memory.Reserve(logical_address_space + MAX_PRG_SIZE + MAX_CHR_SIZE);

    // Alloc the view for the CIRAM and map it to the mirrors
    ciram = memory.CreateView(0, 0x800 * OFFSET);
    memory.MapView(ciram, 0x800 * OFFSET);
    memory.MapView(ciram, 0x800 * 2 * OFFSET);
    memory.MapView(ciram, 0x800 * 3 * OFFSET);

    mmio = memory.CreateView(0x2000 * OFFSET, 0x2000 * OFFSET);

    u32 i = 0;
    for (const auto& slice : prg | ranges::views::sliding(0x1000)) {
        auto mem = memory.CreateView(PRG_OFFSET + (0x1000 * OFFSET * (i++)) , 0x1000 * OFFSET);
        // copy the prg rom data into the memory array
        u32 j = 0;
        for (const auto& b : slice) {
            mem[j++ * 4] = b;
        }
    }
    for (auto& prgview : prgbank) {
        prgview = memory.CreateView(0x6000 * OFFSET + (0x1000 * i++), 0x1000 * OFFSET);
    }
}
