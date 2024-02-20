
#include <range/v3/view.hpp>

#ifdef WIN32

#else
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <cerrno>
#include <cstring>
#endif

#include "ines.h"
#include "virtmem.h"

void FakeVirtualMemory::InitCPUMap(std::span<u8> rawprg) {
    constexpr auto banksize = FakeVirtualMemory::BANK_WINDOW;
    // Load the PRG and CHR into the backing memory
    int i = 0;
    for (const auto &view: rawprg | ranges::views::chunk(banksize)) {
        std::ranges::copy(view, this->prg[i++].begin());
    }
    prg_bank_count = i;

    // Now cpumem the pointers into the memory mapping

    // CIRAM - since its larger than the bank size we need to split it
    u32 addr = 0x0;
    for (; addr < 0x2000; addr += banksize) {
        // Offset bounces between 0 and banksize;
        u32 offset = (addr & banksize);
        cpumem[addr / banksize] = ciram.data() + offset;
        cputag[addr / banksize] = Tag::Read | Tag::Write;
    }

    // MMIO - don't cpumem memory, let the PPU handle these
    for (addr = 0x2000; addr < 0x6000; addr += banksize) {
        cpumem[addr / banksize] = nullptr;
        cputag[addr / banksize] = FakeVirtualMemory::Tag::MMIO;
    }

    // PRG - Map these starting from the end.
    // TODO - let the mapper choose how its mapped. This is really brittle.
    addr = 0x10000 - banksize;
    for (auto& view: prg | ranges::views::slice(0, prg_bank_count) | ranges::views::reverse) {
        cpumem[addr / banksize] = view.data();
        cputag[addr / banksize] = FakeVirtualMemory::Tag::Read;
        addr -= banksize;
        if (addr < 0x6000) {
            break;
        }
    }
}

void FakeVirtualMemory::InitPPUMap(std::span<u8> rawchr, const INES& header) {
    constexpr auto banksize = FakeVirtualMemory::BANK_WINDOW;

    // If the game has chr-rom then it will be nonempty
    if (!rawchr.empty()) {
        int i = 0;
        for (const auto &view: rawchr | ranges::views::chunk(banksize)) {
            std::ranges::copy(view, this->chr[i++].begin());
        }
        chr_bank_count = i;

        u32 addr = 0x0;
        for (auto &view: chr | ranges::views::slice(0, chr_bank_count) | ranges::views::reverse) {
            ppumem[addr / banksize] = view.data();
            chr_pixel_map[addr / banksize] = decoded_pixel_cache[addr / banksize].data();
            u8 tag = Tag::Read;
            pputag[addr / banksize] = tag;
            addr += banksize;
        }

        // Decode the CHR blocks into pixels in a cache
        for (int bank_id=0; bank_id < chr.size(); bank_id++) {
            constexpr int TILE_STRIDE = 16;
            constexpr int PIXEL_STRIDE = TILE_STRIDE * 8;
            const auto& bank = chr[bank_id];
            auto& cache = decoded_pixel_cache[bank_id];
            for (int n=0; n < bank.size(); ++n) {
                u32 offset = n * TILE_STRIDE;
                u32 x = (n % TILE_STRIDE) * 8;
                u32 y = (n / TILE_STRIDE) * 8;
                for (int j=0; j < 8; ++j) {
                    u8 plane0 = bank[offset + j];
                    u8 plane1 = bank[offset + j + 8];
                    for (int k=0; k < 8; ++k) {
                        u8 pixelbit = 7-k;
                        u8 bit0 = (plane0>>pixelbit) & 1;
                        u8 bit1 = ((plane1>>pixelbit) & 1) << 1;
                        u8 color = (bit0 | bit1);
                        cache[x + k + (y + j) * PIXEL_STRIDE] = color;
                    }
                }
            }
        }
    }

    // Map nametables
    switch (header.mirroring) {
    case INES::Mirroring::Vertical:
        ppumem[0x2000 / banksize] = nmt[0].data();
        ppumem[0x2400 / banksize] = nmt[1].data();
        ppumem[0x2800 / banksize] = nmt[0].data();
        ppumem[0x2c00 / banksize] = nmt[1].data();
        pputag[0x2000 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2400 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2800 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2c00 / banksize] = Tag::Read | Tag::Write;
        break;
    case INES::Mirroring::Single:
        ppumem[0x2000 / banksize] = nmt[0].data();
        ppumem[0x2400 / banksize] = nmt[0].data();
        ppumem[0x2800 / banksize] = nmt[0].data();
        ppumem[0x2c00 / banksize] = nmt[0].data();
        pputag[0x2000 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2400 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2800 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2c00 / banksize] = Tag::Read | Tag::Write;
        break;
    case INES::Mirroring::Horizontal:
        ppumem[0x2000 / banksize] = nmt[0].data();
        ppumem[0x2400 / banksize] = nmt[0].data();
        ppumem[0x2800 / banksize] = nmt[1].data();
        ppumem[0x2c00 / banksize] = nmt[1].data();
        pputag[0x2000 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2400 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2800 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2c00 / banksize] = Tag::Read | Tag::Write;
        break;
    case INES::Mirroring::FourWay:
        ppumem[0x2000 / banksize] = nmt[0].data();
        ppumem[0x2400 / banksize] = nmt[1].data();
        ppumem[0x2800 / banksize] = nmt[2].data();
        ppumem[0x2c00 / banksize] = nmt[3].data();
        pputag[0x2000 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2400 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2800 / banksize] = Tag::Read | Tag::Write;
        pputag[0x2c00 / banksize] = Tag::Read | Tag::Write;
        break;
    }
    // Setup the mirrors
    ppumem[0x3000 / banksize] = ppumem[0x2000 / banksize];
    ppumem[0x3400 / banksize] = ppumem[0x2400 / banksize];
    ppumem[0x3800 / banksize] = ppumem[0x2800 / banksize];
    ppumem[0x3c00 / banksize] = ppumem[0x2c00 / banksize];
    pputag[0x3000 / banksize] = pputag[0x2000 / banksize];
    pputag[0x3400 / banksize] = pputag[0x2400 / banksize];
    pputag[0x3800 / banksize] = pputag[0x2800 / banksize];
    pputag[0x3c00 / banksize] = pputag[0x2c00 / banksize];

    // Palette addresses are handled in the PPU IO handler
}

//
//void HostVirtualMemory::Reserve(u32 size) {
//#ifdef WIN32
//
//#else
//    // Create shared mem file that holds the memory
//    auto name = "brutenes_memory";
//    backing_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
//    if (backing_fd == -1) {
//        SPDLOG_WARN("Failed to Reserve BackingFD Error: {}", strerror(errno));
//        return;
//    }
//    shm_unlink(name);
//    if (ftruncate(backing_fd, size) < 0) {
//        return;
//    }
//    // now reserve the address space and get the pointer for where this goes
//    backing = static_cast<u8*>(
//            mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, backing_fd, 0));
//    if (backing == MAP_FAILED) {
//        SPDLOG_WARN("Failed to Reserve Error: {}", strerror(errno));
//        return;
//    }
//#endif
//    this->size = size;
//}
//
//std::span<u8> HostVirtualMemory::CreateView(u32 offset, u32 size) {
//#ifdef WIN32
//
//#else
//    void* mapping = mmap((u8*)backing + offset, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, backing_fd, offset);
//    if (mapping == MAP_FAILED) {
//        SPDLOG_WARN("Failed to CreateView Error: {}", strerror(errno));
//        return std::span<u8>{reinterpret_cast<u8*>(0), 0};
//    }
//    return {reinterpret_cast<u8*>(mapping), size};
//#endif
//}
//
//std::span<u8> HostVirtualMemory::MapView(std::span<u8> view, u32 offset) {
//#ifdef WIN32
//
//#else
//    void* mapping = mmap((u8*)backing + offset, view.size(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, backing_fd, view.data() - (u8*)backing);
//    if (mapping == MAP_FAILED) {
//        SPDLOG_WARN("Failed to MapView Error: {}", strerror(errno));
//        return std::span<u8>{reinterpret_cast<u8*>(0), 0};
//    }
//#endif
//    return {reinterpret_cast<u8*>(mapping), view.size()};
//}
//
