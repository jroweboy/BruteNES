

#ifndef BRUTENES_VIRTMEM_H
#define BRUTENES_VIRTMEM_H

#include <array>
#include <span>
#include <vector>

#include "common.h"

class FakeVirtualMemory {
public:
    static constexpr auto BANK_WINDOW = 0x400;
    static constexpr auto ADDRESS_SPACE = 0x10000;
    static constexpr auto BANK_COUNT = ADDRESS_SPACE / BANK_WINDOW;
    static constexpr auto CIRAM_SIZE = 0x800;

    enum Tag : u8 {
        Unmapped = 0,
        Read     = 1 << 0,
        Write    = 1 << 1,
        MMIO     = 1 << 2,
    };

//    void Reserve(u32 size);
//
//    std::span<u8> CreateView(u32 offset, u32 size);
//    std::span<u8> MapView(std::span<u8> view, u32 offset);

    std::array<u8, BANK_COUNT> attr{};
    std::array<u8*, BANK_COUNT> map{};

    std::array<u8, CIRAM_SIZE> ciram{};
    std::vector<std::array<u8, BANK_WINDOW>> prg{};
    std::vector<std::array<u8, BANK_WINDOW>> chr{};

private:
};

class HostVirtualMemory {
public:
    void Reserve(u32 size);

    std::span<u8> CreateView(u32 offset, u32 size);
    std::span<u8> MapView(std::span<u8> view, u32 offset);

    template <typename T = u8>
    std::span<T> Span() const {
        return {static_cast<T*>(backing), size / sizeof(T)};
    }

    template <typename T = u8>
    explicit operator std::span<T>() const {
        return Span<T>();
    }

private:

    int backing_fd{};
    void* backing{};
    u32 size{};
};


#endif //BRUTENES_VIRTMEM_H
