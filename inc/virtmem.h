

#ifndef BRUTENES_VIRTMEM_H
#define BRUTENES_VIRTMEM_H

#include <span>

#include "common.h"

class VirtualMemory {
public:
    void Reserve(u32 size);

    std::span<u8> CreateView(u32 offset, u32 size);
    std::span<u8> MapView(std::span<u8> view, u32 offset);
private:
    int backing;
};


#endif //BRUTENES_VIRTMEM_H
