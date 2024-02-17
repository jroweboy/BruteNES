
#ifdef WIN32

#else
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#endif

#include "virtmem.h"

void VirtualMemory::Reserve(u32 size) {
#ifdef WIN32

#else
    // Create shared mem file that holds the memory
    auto name = "brutenes_memory";
    backing_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (backing_fd == -1) {
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
        return;
    }
#endif
    this->size = size;
}

std::span<u8> VirtualMemory::CreateView(u32 offset, u32 size) {
#ifdef WIN32

#else
    void* mapping = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, backing_fd, offset);
    if (mapping == MAP_FAILED) {
        return std::span<u8>{reinterpret_cast<u8*>(0), 0};
    }
    return {reinterpret_cast<u8*>(mapping), size};
#endif
}

std::span<u8> VirtualMemory::MapView(std::span<u8> view, u32 offset) {
#ifdef WIN32

#else
    void* mapping = mmap(view.data(), view.size(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, backing_fd, offset);
    if (mapping == MAP_FAILED) {
        return std::span<u8>{reinterpret_cast<u8*>(0), 0};
    }
#endif
    return {reinterpret_cast<u8*>(mapping), view.size()};
}

