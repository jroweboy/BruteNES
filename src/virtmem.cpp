
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
        attr[addr / banksize] = Tag::Read | Tag::Write;
    }

    // MMIO - don't cpumem memory, let the PPU handle these
    for (; addr < 0x6000; addr += banksize) {
        cpumem[addr / banksize] = nullptr;
        attr[addr / banksize] = FakeVirtualMemory::Tag::MMIO;
    }

    // PRG - Map these starting from the end.
    // TODO - let the mapper choose how its mapped. This is really brittle.
    addr = 0x10000 - banksize;
    for (auto& view: prg | ranges::views::slice(0, prg_bank_count) | ranges::views::reverse) {
        cpumem[addr / banksize] = view.data();
        attr[addr / banksize] = FakeVirtualMemory::Tag::Read;
        addr -= banksize;
    }
}

void FakeVirtualMemory::InitPPUMap(std::span<u8> rawchr) {
    constexpr auto banksize = FakeVirtualMemory::BANK_WINDOW;

    int i = 0;
    for (const auto &view: rawchr | ranges::views::chunk(banksize)) {
        std::ranges::copy(view, this->chr[i++].begin());
    }
    chr_bank_count = i;

    u32 addr = 0x0;
    for (auto& view: chr | ranges::views::slice(0, prg_bank_count) | ranges::views::reverse) {
        ppumem[addr / banksize] = view.data();
        attr[addr / banksize] = FakeVirtualMemory::Tag::Read;
        addr -= banksize;
    }
}


void HostVirtualMemory::Reserve(u32 size) {
#ifdef WIN32

#else
    // Create shared mem file that holds the memory
    auto name = "brutenes_memory";
    backing_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (backing_fd == -1) {
        SPDLOG_WARN("Failed to Reserve BackingFD Error: {}", strerror(errno));
        return;
    }
    shm_unlink(name);
    if (ftruncate(backing_fd, size) < 0) {
        return;
    }
    // now reserve the address space and get the pointer for where this goes
    backing = static_cast<u8*>(
            mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, backing_fd, 0));
    if (backing == MAP_FAILED) {
        SPDLOG_WARN("Failed to Reserve Error: {}", strerror(errno));
        return;
    }
#endif
    this->size = size;
}

std::span<u8> HostVirtualMemory::CreateView(u32 offset, u32 size) {
#ifdef WIN32

#else
    void* mapping = mmap((u8*)backing + offset, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, backing_fd, offset);
    if (mapping == MAP_FAILED) {
        SPDLOG_WARN("Failed to CreateView Error: {}", strerror(errno));
        return std::span<u8>{reinterpret_cast<u8*>(0), 0};
    }
    return {reinterpret_cast<u8*>(mapping), size};
#endif
}

std::span<u8> HostVirtualMemory::MapView(std::span<u8> view, u32 offset) {
#ifdef WIN32

#else
    void* mapping = mmap((u8*)backing + offset, view.size(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, backing_fd, view.data() - (u8*)backing);
    if (mapping == MAP_FAILED) {
        SPDLOG_WARN("Failed to MapView Error: {}", strerror(errno));
        return std::span<u8>{reinterpret_cast<u8*>(0), 0};
    }
#endif
    return {reinterpret_cast<u8*>(mapping), view.size()};
}

