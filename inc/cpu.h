
#ifndef BRUTENES_CPU_H
#define BRUTENES_CPU_H

#include <array>
#include <vector>
#include <bit>

#include "common.h"

class Bus;
class CPU;

class Interpreter {
public:
    Interpreter(Bus& bus, CPU& cpu) : bus(bus), cpu(cpu) {}

    u32 RunBlock(u32 max_cycles);
private:
    Bus& bus;
    CPU& cpu;

};

class CPU {
public:
    CPU() = delete;
    explicit CPU(Bus& bus) : bus(bus), interpreter(bus, *this) { Reset(); }

    enum Flags {
        C = 1 << 0,
        D = 1 << 1,
        I = 1 << 2,
        Z = 1 << 3,
        B = 1 << 4,
        U = 1 << 5,
        V = 1 << 6,
        N = 1 << 7,
    };

    constexpr void SetNZ(u8 value) {
        P = (P & ~(Flags::N | Flags::Z))
                | ((value == 0) << std::countr_zero((u8)Flags::Z))
                | ((value & 0x80));
    }

    template<Flags F>
    constexpr void SetFlag(bool result) {
        P = (P & ~F) | (result << std::countr_zero((u8)F));
    }

    inline void PushStack(u8 value);
    inline u8 PopStack();

    void Reset();

    // Runs until the expected cycle count or MMIO page is accessed
    u32 RunFor(u64 cycles);

    u16 PC{};
    u8 SP{};
    u8 P{};
    u8 A{};
    u8 X{};
    u8 Y{};

private:
    Bus& bus;
    Interpreter interpreter;
};


#endif
