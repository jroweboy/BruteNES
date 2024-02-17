

#ifndef BRUTENES_VIRTMEM_H
#define BRUTENES_VIRTMEM_H

#include <span>

#include "common.h"

class VirtualMemory {
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

    int backing_fd;
    void* backing;
    u32 size;
};


#endif //BRUTENES_VIRTMEM_H
