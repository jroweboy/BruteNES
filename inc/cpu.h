
#ifndef BRUTENES_CPU_H
#define BRUTENES_CPU_H

#include <array>
#include <vector>
#include <bit>

#include "bus.h"
#include "common.h"

class CPU;
class PPU;
class Scheduler;

class Interpreter {
public:
    Interpreter(Bus& bus, CPU& cpu, Scheduler& timing)
        : bus(bus), cpu(cpu), timing(timing) {}

    u32 RunBlock(u32 max_cycles, bool use_ppu_cache);
private:
    Bus& bus;
    CPU& cpu;
    Scheduler& timing;

    u16 prev_pc{};
};

class CPU {
public:
    constexpr static u16 NMIVector = 0xfffa;
    constexpr static u16 ResetVector = 0xfffc;
    constexpr static u16 IRQVector = 0xfffe;
    CPU() = delete;
    explicit CPU(Bus& bus, Scheduler& timing) : bus(bus), timing(timing), interpreter(bus, *this, timing) { Reset(); }

    enum Flags {
        C = 1 << 0,
        Z = 1 << 1,
        I = 1 << 2,
        D = 1 << 3,
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
        P = (P & ~F) | ((result) << std::countr_zero((u8)F));
    }

    inline void PushStack(u8 value) {
        bus.Write8(0x100 | SP--, value);
    }
    inline u8 PopStack() {
        return bus.Read8(0x100 | ++SP);
    }

    void Reset();

    // Runs until the expected cycle count or MMIO page is accessed
    u32 RunFor(u64 cycles, bool use_ppu_cache);

    u16 PC{};
    u8 SP{};
    u8 P{};
    u8 A{};
    u8 X{};
    u8 Y{};
    bool pending_irq{};
    bool pending_nmi{};

private:

    Bus& bus;
    Scheduler& timing;
    Interpreter interpreter;
};


#endif
