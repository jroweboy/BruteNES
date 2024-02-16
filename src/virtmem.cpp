
#ifdef WIN32

#else
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#endif

#include <format>
#include <unistd.h>

#include "virtmem.h"

void VirtualMemory::Reserve(u32 size) {
//    auto name = std::format("");
#ifdef WIN32

#else

    auto name = "testing";
    backing = shm_open(name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
    if (backing == -1)
    {
        return;
    }
    shm_unlink(name);
    if (ftruncate(backing, size) < 0) {
        return ;
    }

}

std::span<u8> VirtualMemory::CreateView(u32 offset, u32 size) {
#ifdef WIN32

#else
    void* mapping = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, backing, offset);
    if (mapping == MAP_FAILED)
    {
        return std::span<u8>{reinterpret_cast<u8*>(0), 0};
    }
    return {reinterpret_cast<u8*>(mapping), size};
#endif
}

std::span<u8> VirtualMemory::MapView(std::span<u8> view, u32 offset) {
#ifdef WIN32

#else
    void* mapping = mmap(view.data(), view.size(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, backing, offset);
    if (mapping == MAP_FAILED) {
        return std::span<u8>{reinterpret_cast<u8*>(0), 0};
    }
#endif
    return {reinterpret_cast<u8*>(mapping), view.size()};
}



