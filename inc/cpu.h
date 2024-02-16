
#ifndef BRUTENES_CPU_H
#define BRUTENES_CPU_H

#include <array>
#include <vector>

#include "common.h"
#include "bus.h"

class Bus;

class CPU {
public:
    CPU(Bus& memory) : memory(memory) {}

    u8 Read(u16 addr);
    void Write(u16 addr, u8 value);

private:
    Bus& memory;
};

#endif
