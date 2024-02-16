
#include "cpu.h"
#include "bus.h"

u8 CPU::Read(u16 addr) {
    if ((addr & 0x8000) != 0) {

//        return memory.slab[addr + memory.prg_offset + ];
    }
//    if ((addr &)) {

//    }
    return 0;
}

void CPU::Write(u16 addr, u8 value) {

}
