
#include <signal.h>

#include "cpu.h"
#include "bus.h"


void CPU::Reset() {
    // https://www.nesdev.org/wiki/CPU_power_up_state
    PC = bus.Read16(ResetVector);
//    PC = 0xc000;
    SP = 0xfd;
    P = 0x34;
    A = 0;
    X = 0;
    Y = 0;
}

u32 CPU::RunFor(u64 cycles, bool use_ppu_cache) {
    u32 count = interpreter.RunBlock(cycles, use_ppu_cache);
    return count;
}

// Copied from https://github.com/bheisler/Corrosion/blob/5ca2b3a03825c3d58623df774a8596de32b46812/src/cpu/mod.rs#L356
static std::array<u8, 256> cycle_lut = {
    /*0x00*/ 7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
    /*0x10*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x20*/ 6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
    /*0x30*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x40*/ 6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
    /*0x50*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x60*/ 6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
    /*0x70*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /*0x90*/ 2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
    /*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    /*0xB0*/ 2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
    /*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /*0xD0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    /*0xE0*/ 2,6,3,8,3,3,5,5,2,2,2,2,4,4,6,6,
    /*0xF0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
};


static std::array<std::string, 256> inst_name_lut = {{
        "BRK_IMM", "ORA_INX", "STP_IMP", "SLO_INX", "NOP_ZPA", "ORA_ZPA", "ASL_ZPA", "SLO_ZPA",
        "PHP_IMP", "ORA_IMM", "ASL_IMP", "ANC_IMM", "NOP_ABS", "ORA_ABS", "ASL_ABS", "SLO_ABS",
        "BPL_REL", "ORA_INY", "STP_IMP", "SLO_INY", "NOP_ZPX", "ORA_ZPX", "ASL_ZPX", "SLO_ZPX",
        "CLC_IMP", "ORA_ABY", "NOP_IMP", "SLO_ABY", "NOP_ABX", "ORA_ABX", "ASL_ABX", "SLO_ABX",
        "JSR_ABS", "AND_INX", "STP_IMP", "RLA_INX", "BIT_ZPA", "AND_ZPA", "ROL_ZPA", "RLA_ZPA",
        "PLP_IMP", "AND_IMM", "ROL_IMP", "ANC_IMM", "BIT_ABS", "AND_ABS", "ROL_ABS", "RLA_ABS",
        "BMI_REL", "AND_INY", "STP_IMP", "RLA_INY", "NOP_ZPX", "AND_ZPX", "ROL_ZPX", "RLA_ZPX",
        "SEC_IMP", "AND_ABY", "NOP_IMP", "RLA_ABY", "NOP_ABX", "AND_ABX", "ROL_ABX", "RLA_ABX",
        "RTI_IMP", "EOR_INX", "STP_IMP", "SRE_INX", "NOP_ZPA", "EOR_ZPA", "LSR_ZPA", "SRE_ZPA",
        "PHA_IMP", "EOR_IMM", "LSR_IMP", "ALR_IMM", "JMP_ABS", "EOR_ABS", "LSR_ABS", "SRE_ABS",
        "BVC_REL", "EOR_INY", "STP_IMP", "SRE_INY", "NOP_ZPX", "EOR_ZPX", "LSR_ZPX", "SRE_ZPX",
        "CLI_IMP", "EOR_ABY", "NOP_IMP", "SRE_ABY", "NOP_ABX", "EOR_ABX", "LSR_ABX", "SRE_ABX",
        "RTS_IMP", "ADC_INX", "STP_IMP", "RRA_INX", "NOP_ZPA", "ADC_ZPA", "ROR_ZPA", "RRA_ZPA",
        "PLA_IMP", "ADC_IMM", "ROR_IMP", "ARR_IMM", "JMP_IND", "ADC_ABS", "ROR_ABS", "RRA_ABS",
        "BVS_REL", "ADC_INY", "STP_IMP", "RRA_INY", "NOP_ZPX", "ADC_ZPX", "ROR_ZPX", "RRA_ZPX",
        "SEI_IMP", "ADC_ABY", "NOP_IMP", "RRA_ABY", "NOP_ABX", "ADC_ABX", "ROR_ABX", "RRA_ABX",
        "NOP_IMM", "STA_INX", "NOP_IMM", "SAX_INX", "STY_ZPA", "STA_ZPA", "STX_ZPA", "SAX_ZPA",
        "DEY_IMP", "NOP_IMM", "TXA_IMP", "XAA_IMM", "STY_ABS", "STA_ABS", "STX_ABS", "SAX_ABS",
        "BCC_REL", "STA_INY", "STP_IMP", "AHX_INY", "STY_ZPX", "STA_ZPX", "STX_ZPY", "SAX_ZPY",
        "TYA_IMP", "STA_ABY", "TXS_IMP", "TAS_ABY", "SHY_ABX", "STA_ABX", "SHX_ABY", "AHX_ABY",
        "LDY_IMM", "LDA_INX", "LDX_IMM", "LAX_INX", "LDY_ZPA", "LDA_ZPA", "LDX_ZPA", "LAX_ZPA",
        "TAY_IMP", "LDA_IMM", "TAX_IMP", "LAX_IMM", "LDY_ABS", "LDA_ABS", "LDX_ABS", "LAX_ABS",
        "BCS_REL", "LDA_INY", "STP_IMP", "LAX_INY", "LDY_ZPX", "LDA_ZPX", "LDX_ZPY", "LAX_ZPY",
        "CLV_IMP", "LDA_ABY", "TSX_IMP", "LAS_ABY", "LDY_ABX", "LDA_ABX", "LDX_ABY", "LAX_ABY",
        "CPY_IMM", "CMP_INX", "NOP_IMM", "DCP_INX", "CPY_ZPA", "CMP_ZPA", "DEC_ZPA", "DCP_ZPA",
        "INY_IMP", "CMP_IMM", "DEX_IMP", "AXS_IMM", "CPY_ABS", "CMP_ABS", "DEC_ABS", "DCP_ABS",
        "BNE_REL", "CMP_INY", "STP_IMP", "DCP_INY", "NOP_ZPX", "CMP_ZPX", "DEC_ZPX", "DCP_ZPX",
        "CLD_IMP", "CMP_ABY", "NOP_IMP", "DCP_ABY", "NOP_ABX", "CMP_ABX", "DEC_ABX", "DCP_ABX",
        "CPX_IMM", "SBC_INX", "NOP_IMM", "ISC_INX", "CPX_ZPA", "SBC_ZPA", "INC_ZPA", "ISC_ZPA",
        "INX_IMP", "SBC_IMM", "NOP_IMP", "SBC_IMM", "CPX_ABS", "SBC_ABS", "INC_ABS", "ISC_ABS",
        "BEQ_REL", "SBC_INY", "STP_IMP", "ISC_INY", "NOP_ZPX", "SBC_ZPX", "INC_ZPX", "ISC_ZPX",
        "SED_IMP", "SBC_ABY", "NOP_IMP", "ISC_ABY", "NOP_ABX", "SBC_ABX", "INC_ABX", "ISC_ABX",
}};


#define TRACE_LOG() \
    SPDLOG_INFO("${:4X}: {} ${:X} a${:2x} x${:2x} y${:2x} SP${:2x} P${:2x} -> {} @ ${:4X}", \
                prev_pc, inst_name_lut[prev_idx], operand, \
                cpu.A, cpu.X, cpu.Y, cpu.SP, cpu.P, \
                inst_name_lut[inst_idx], cpu.PC);

#undef TRACE_LOG
#define TRACE_LOG() ;

// I'm so sorry.
#define NOOP(inner) ;
#define NOOP8(inner) u8 value = inner;
#define NOOP16(inner) u16 value = inner;
//#define READ8(inner) u8 value = bus.Read8(inner);

#define CHECKED_READ_8(inner) u8 value; {   \
    if (!bus.CheckedRead8(inner, value)) {       \
        if (current_cycles != 0)                 \
            goto MMIO;                           \
        if (inner >= 0x2000 && inner < 0x4000) { \
            value = bus.ReadPPURegister(inner); \
        } else if (inner >= 0x4000 && inner < 0x4018) { \
            value = bus.ReadAPURegister(inner); \
        } else {                                \
            /* TODO mapper MMIO */              \
            value = bus.OpenBus();              \
        }                                       \
    }                                           \
}

#define CHECKED_WRITE_8(inner, value) {   \
    if (!bus.CheckedWrite8(inner, value)) {  \
        if (inner >= 0x2000 && inner < 0x4000) { \
            if (!use_ppu_cache && current_cycles != 0) { \
                /* write can't happen until the PPU is caught up*/ \
                goto MMIO;                       \
            }                                    \
            bus.WritePPURegister(inner, value, use_ppu_cache); \
        } else if (inner >= 0x4000 && inner < 0x4018) { \
            /*the only write we can't support is starting dmc*/ \
            if ((value & 0x10) && inner == 0x4015) { \
                goto MMIO;                         \
            }                                      \
            bus.WriteAPURegister(value, inner, true); \
        } else {                            \
            goto MMIO;                      \
        }                                   \
    }                                       \
}

#define FETCH_NEXT {                        \
        u8 prev_idx = inst_idx;             \
        {                                   \
            CHECKED_READ_8(cpu.PC)              \
            inst_idx = value;                   \
        }                                   \
        TRACE_LOG();                        \
        prev_pc = cpu.PC++;                 \
    }


#define PAGE_CROSS_CHECK(reg) (((operand + cpu.reg) & 0xff) < cpu.reg)

#define GOTO_NEXT(page_crossed) {                                  \
        current_cycles += cycle_lut[inst_idx] + page_crossed;      \
        if (current_cycles >= max_cycles)                          \
            goto END;                                              \
        FETCH_NEXT                                                 \
        goto* inst_lut[inst_idx];                                  \
    }

#define COMPARE_OP(reg) { \
    cpu.SetFlag<CPU::Flags::C>(reg >= value);                      \
    cpu.SetFlag<CPU::Flags::Z>(reg == value);                \
    cpu.SetFlag<CPU::Flags::N>(((reg - value) & 0x80) != 0 );      \
}

#define DECODE_REL(name, condition)                                \
name##_REL: {                                                      \
        operand = bus.Read8(cpu.PC++);                             \
        u8 penalty = 0;                                            \
        if (condition) {                                           \
            u16 oldPC = cpu.PC;                                    \
            cpu.PC += (s8) operand;                                \
            penalty = 1 + ((oldPC & 0xff00) == (cpu.PC & 0xff00)); \
        }                                                          \
        GOTO_NEXT(penalty)                                         \
    }
#define DECODE_IMP(name, code)                   \
name##_IMP: {                                    \
        code                                     \
        GOTO_NEXT(0)                             \
    }
#define DECODE_IMM(name, OP, code)               \
name##_IMM: {                                    \
        operand = bus.Read8(cpu.PC++);           \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(0)                             \
    }
#define DECODE_ABS(name, OP, code)               \
name##_ABS: {                                    \
        operand = bus.Read16(cpu.PC);            \
        cpu.PC += 2;                             \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(0)                             \
    }
#define DECODE_ABX(name, OP, code, page_cross)   \
name##_ABX: {                                    \
        operand = bus.Read16(cpu.PC) + cpu.X;    \
        cpu.PC += 2;                             \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(page_cross)                    \
    }
#define DECODE_ABY(name, OP, code, page_cross)   \
name##_ABY: {                                    \
        operand = bus.Read16(cpu.PC) + cpu.Y;    \
        cpu.PC += 2;                             \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(page_cross)                    \
    }
#define DECODE_INX(name, OP, code)               \
name##_INX: {                                    \
        operand = bus.Read8(cpu.PC++) + cpu.X;   \
        if ((operand & 0xff) == 0xff) {          \
            u8 lo = bus.Read8(operand);          \
            u8 hi = bus.Read8(operand - 0xff);   \
            operand = (hi << 8) | lo;            \
        } else {                                 \
            operand = bus.Read16(operand);       \
        }                                        \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(0)                             \
    }
#define DECODE_INY(name, OP, code, page_cross)   \
name##_INY: {                                    \
        operand = bus.Read8(cpu.PC++);           \
        if ((operand & 0xff) == 0xff) {          \
            u8 lo = bus.Read8(0xff);             \
            u8 hi = bus.Read8(0);                \
            operand = (hi << 8) | lo;            \
        } else {                                 \
            operand = bus.Read16(operand);       \
        }                                        \
        operand += cpu.Y;                        \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(page_cross)                    \
    }
#define DECODE_ZPA(name, OP, code)               \
name##_ZPA: {                                    \
        operand = bus.Read8(cpu.PC++);           \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(0)                             \
    }
#define DECODE_ZPX(name, OP, code)               \
name##_ZPX: {                                    \
        operand = bus.Read8(cpu.PC++) + cpu.X;   \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(0)                             \
    }
#define DECODE_ZPY(name, OP, code)               \
name##_ZPY: {                                    \
        operand = bus.Read8(cpu.PC++) + cpu.Y;   \
        OP(operand)                              \
        code                                     \
        GOTO_NEXT(0)                             \
    }

u32 Interpreter::RunBlock(u32 max_cycles, bool use_ppu_cache) {

    u32 current_cycles = 0;
    u16 operand;
//    u16 result;
    u8 inst_idx{};

/*
0x00   BRK         ORA (d,x)   STP         SLO (d,x)   NOP d       ORA d       ASL d       SLO d
0x08   PHP         ORA #i      ASL         ANC #i      NOP a       ORA a       ASL a       SLO a
0x10   BPL *+d     ORA (d),y   STP         SLO (d),y   NOP d,x     ORA d,x     ASL d,x     SLO d,x
0x18   CLC         ORA a,y     NOP         SLO a,y     NOP a,x     ORA a,x     ASL a,x     SLO a,x
0x20   JSR a       AND (d,x)   STP         RLA (d,x)   BIT d       AND d       ROL d       RLA d
0x28   PLP         AND #i      ROL         ANC #i      BIT a       AND a       ROL a       RLA a
0x30   BMI *+d     AND (d),y   STP         RLA (d),y   NOP d,x     AND d,x     ROL d,x     RLA d,x
0x38   SEC         AND a,y     NOP         RLA a,y     NOP a,x     AND a,x     ROL a,x     RLA a,x
0x40   RTI         EOR (d,x)   STP         SRE (d,x)   NOP d       EOR d       LSR d       SRE d
0x48   PHA         EOR #i      LSR         ALR #i      JMP a       EOR a       LSR a       SRE a
0x50   BVC *+d     EOR (d),y   STP         SRE (d),y   NOP d,x     EOR d,x     LSR d,x     SRE d,x
0x58   CLI         EOR a,y     NOP         SRE a,y     NOP a,x     EOR a,x     LSR a,x     SRE a,x
0x60   RTS         ADC (d,x)   STP         RRA (d,x)   NOP d       ADC d       ROR d       RRA d
0x68   PLA         ADC #i      ROR         ARR #i      JMP (a)     ADC a       ROR a       RRA a
0x70   BVS *+d     ADC (d),y   STP         RRA (d),y   NOP d,x     ADC d,x     ROR d,x     RRA d,x
0x78   SEI         ADC a,y     NOP         RRA a,y     NOP a,x     ADC a,x     ROR a,x     RRA a,x
0x80   NOP #i      STA (d,x)   NOP #i      SAX (d,x)   STY d       STA d       STX d       SAX d
0x88   DEY         NOP #i      TXA         XAA #i      STY a       STA a       STX a       SAX a
0x90   BCC *+d     STA (d),y   STP         AHX (d),y   STY d,x     STA d,x     STX d,y     SAX d,y
0x98   TYA         STA a,y     TXS         TAS a,y     SHY a,x     STA a,x     SHX a,y     AHX a,y
0xa0   LDY #i      LDA (d,x)   LDX #i      LAX (d,x)   LDY d       LDA d       LDX d       LAX d
0xa8   TAY         LDA #i      TAX         LAX #i      LDY a       LDA a       LDX a       LAX a
0xb0   BCS *+d     LDA (d),y   STP         LAX (d),y   LDY d,x     LDA d,x     LDX d,y     LAX d,y
0xb8   CLV         LDA a,y     TSX         LAS a,y     LDY a,x     LDA a,x     LDX a,y     LAX a,y
0xc0   CPY #i      CMP (d,x)   NOP #i      DCP (d,x)   CPY d       CMP d       DEC d       DCP d
0xc8   INY         CMP #i      DEX         AXS #i      CPY a       CMP a       DEC a       DCP a
0xd0   BNE *+d     CMP (d),y   STP         DCP (d),y   NOP d,x     CMP d,x     DEC d,x     DCP d,x
0xd8   CLD         CMP a,y     NOP         DCP a,y     NOP a,x     CMP a,x     DEC a,x     DCP a,x
0xe0   CPX #i      SBC (d,x)   NOP #i      ISC (d,x)   CPX d       SBC d       INC d       ISC d
0xe8   INX         SBC #i      NOP         SBC #i      CPX a       SBC a       INC a       ISC a
0xf0   BEQ *+d     SBC (d),y   STP         ISC (d),y   NOP d,x     SBC d,x     INC d,x     ISC d,x
0xf8   SED         SBC a,y     NOP         ISC a,y     NOP a,x     SBC a,x     INC a,x     ISC a,x
*/
    static std::array<void*, 256> inst_lut = {
        &&BRK_IMM, &&ORA_INX, &&STP_IMP, &&SLO_INX, &&NOP_ZPA, &&ORA_ZPA, &&ASL_ZPA, &&SLO_ZPA,
        &&PHP_IMP, &&ORA_IMM, &&ASL_IMP, &&ANC_IMM, &&NOP_ABS, &&ORA_ABS, &&ASL_ABS, &&SLO_ABS,
        &&BPL_REL, &&ORA_INY, &&STP_IMP, &&SLO_INY, &&NOP_ZPX, &&ORA_ZPX, &&ASL_ZPX, &&SLO_ZPX,
        &&CLC_IMP, &&ORA_ABY, &&NOP_IMP, &&SLO_ABY, &&NOP_ABX, &&ORA_ABX, &&ASL_ABX, &&SLO_ABX,
        &&JSR_ABS, &&AND_INX, &&STP_IMP, &&RLA_INX, &&BIT_ZPA, &&AND_ZPA, &&ROL_ZPA, &&RLA_ZPA,
        &&PLP_IMP, &&AND_IMM, &&ROL_IMP, &&ANC_IMM, &&BIT_ABS, &&AND_ABS, &&ROL_ABS, &&RLA_ABS,
        &&BMI_REL, &&AND_INY, &&STP_IMP, &&RLA_INY, &&NOP_ZPX, &&AND_ZPX, &&ROL_ZPX, &&RLA_ZPX,
        &&SEC_IMP, &&AND_ABY, &&NOP_IMP, &&RLA_ABY, &&NOP_ABX, &&AND_ABX, &&ROL_ABX, &&RLA_ABX,
        &&RTI_IMP, &&EOR_INX, &&STP_IMP, &&SRE_INX, &&NOP_ZPA, &&EOR_ZPA, &&LSR_ZPA, &&SRE_ZPA,
        &&PHA_IMP, &&EOR_IMM, &&LSR_IMP, &&ALR_IMM, &&JMP_ABS, &&EOR_ABS, &&LSR_ABS, &&SRE_ABS,
        &&BVC_REL, &&EOR_INY, &&STP_IMP, &&SRE_INY, &&NOP_ZPX, &&EOR_ZPX, &&LSR_ZPX, &&SRE_ZPX,
        &&CLI_IMP, &&EOR_ABY, &&NOP_IMP, &&SRE_ABY, &&NOP_ABX, &&EOR_ABX, &&LSR_ABX, &&SRE_ABX,
        &&RTS_IMP, &&ADC_INX, &&STP_IMP, &&RRA_INX, &&NOP_ZPA, &&ADC_ZPA, &&ROR_ZPA, &&RRA_ZPA,
        &&PLA_IMP, &&ADC_IMM, &&ROR_IMP, &&ARR_IMM, &&JMP_IND, &&ADC_ABS, &&ROR_ABS, &&RRA_ABS,
        &&BVS_REL, &&ADC_INY, &&STP_IMP, &&RRA_INY, &&NOP_ZPX, &&ADC_ZPX, &&ROR_ZPX, &&RRA_ZPX,
        &&SEI_IMP, &&ADC_ABY, &&NOP_IMP, &&RRA_ABY, &&NOP_ABX, &&ADC_ABX, &&ROR_ABX, &&RRA_ABX,
        &&NOP_IMM, &&STA_INX, &&NOP_IMM, &&SAX_INX, &&STY_ZPA, &&STA_ZPA, &&STX_ZPA, &&SAX_ZPA,
        &&DEY_IMP, &&NOP_IMM, &&TXA_IMP, &&XAA_IMM, &&STY_ABS, &&STA_ABS, &&STX_ABS, &&SAX_ABS,
        &&BCC_REL, &&STA_INY, &&STP_IMP, &&AHX_INY, &&STY_ZPX, &&STA_ZPX, &&STX_ZPY, &&SAX_ZPY,
        &&TYA_IMP, &&STA_ABY, &&TXS_IMP, &&TAS_ABY, &&SHY_ABX, &&STA_ABX, &&SHX_ABY, &&AHX_ABY,
        &&LDY_IMM, &&LDA_INX, &&LDX_IMM, &&LAX_INX, &&LDY_ZPA, &&LDA_ZPA, &&LDX_ZPA, &&LAX_ZPA,
        &&TAY_IMP, &&LDA_IMM, &&TAX_IMP, &&LAX_IMM, &&LDY_ABS, &&LDA_ABS, &&LDX_ABS, &&LAX_ABS,
        &&BCS_REL, &&LDA_INY, &&STP_IMP, &&LAX_INY, &&LDY_ZPX, &&LDA_ZPX, &&LDX_ZPY, &&LAX_ZPY,
        &&CLV_IMP, &&LDA_ABY, &&TSX_IMP, &&LAS_ABY, &&LDY_ABX, &&LDA_ABX, &&LDX_ABY, &&LAX_ABY,
        &&CPY_IMM, &&CMP_INX, &&NOP_IMM, &&DCP_INX, &&CPY_ZPA, &&CMP_ZPA, &&DEC_ZPA, &&DCP_ZPA,
        &&INY_IMP, &&CMP_IMM, &&DEX_IMP, &&AXS_IMM, &&CPY_ABS, &&CMP_ABS, &&DEC_ABS, &&DCP_ABS,
        &&BNE_REL, &&CMP_INY, &&STP_IMP, &&DCP_INY, &&NOP_ZPX, &&CMP_ZPX, &&DEC_ZPX, &&DCP_ZPX,
        &&CLD_IMP, &&CMP_ABY, &&NOP_IMP, &&DCP_ABY, &&NOP_ABX, &&CMP_ABX, &&DEC_ABX, &&DCP_ABX,
        &&CPX_IMM, &&SBC_INX, &&NOP_IMM, &&ISC_INX, &&CPX_ZPA, &&SBC_ZPA, &&INC_ZPA, &&ISC_ZPA,
        &&INX_IMP, &&SBC_IMM, &&NOP_IMP, &&SBC_IMM, &&CPX_ABS, &&SBC_ABS, &&INC_ABS, &&ISC_ABS,
        &&BEQ_REL, &&SBC_INY, &&STP_IMP, &&ISC_INY, &&NOP_ZPX, &&SBC_ZPX, &&INC_ZPX, &&ISC_ZPX,
        &&SED_IMP, &&SBC_ABY, &&NOP_IMP, &&ISC_ABY, &&NOP_ABX, &&SBC_ABX, &&INC_ABX, &&ISC_ABX,
    };

    // Start the interpreter
    FETCH_NEXT
    goto* inst_lut[inst_idx];

    // Control OPCODES
    DECODE_IMP(NOP, {})
    DECODE_ZPA(BIT, CHECKED_READ_8, { cpu.SetNZ(value); cpu.SetFlag<CPU::Flags::V>((value & (1 << 6)) != 0); })
    DECODE_ABS(BIT, CHECKED_READ_8, { cpu.SetNZ(value); cpu.SetFlag<CPU::Flags::V>((value & (1 << 6)) != 0); })
    DECODE_IMM(BRK, NOOP, { cpu.pending_irq = true; goto END; })
    DECODE_IMP(RTS, {
        u16 addr = cpu.PopStack();
        addr |= cpu.PopStack() << 8;
        cpu.PC = addr + 1;
    })
    DECODE_IMP(RTI, {
        cpu.P = cpu.PopStack();
        u16 addr = cpu.PopStack();
        addr |= cpu.PopStack() << 8;
        cpu.PC = addr;
    })

    DECODE_IMP(TAX, { cpu.X = cpu.A; cpu.SetNZ(cpu.A); })
    DECODE_IMP(TXA, { cpu.A = cpu.X; cpu.SetNZ(cpu.A); })
    DECODE_IMP(TAY, { cpu.Y = cpu.A; cpu.SetNZ(cpu.A); })
    DECODE_IMP(TYA, { cpu.A = cpu.Y; cpu.SetNZ(cpu.A); })
    DECODE_IMP(TSX, { cpu.X = cpu.SP; cpu.SetNZ(cpu.X); })
    DECODE_IMP(TXS, { cpu.SP = cpu.X; })
    DECODE_IMP(PHP, { cpu.PushStack(cpu.P); })
    DECODE_IMP(PLP, { cpu.P = cpu.PopStack(); })
    DECODE_IMP(PHA, { cpu.PushStack(cpu.A); })
    DECODE_IMP(PLA, { cpu.A = cpu.PopStack(); })

    DECODE_IMP(CLC, { cpu.SetFlag<CPU::Flags::C>(false); })
    DECODE_IMP(SEC, { cpu.SetFlag<CPU::Flags::C>(true); })
    DECODE_IMP(CLI, { cpu.SetFlag<CPU::Flags::I>(false); })
    DECODE_IMP(SEI, { cpu.SetFlag<CPU::Flags::I>(true); })
    DECODE_IMP(CLD, { cpu.SetFlag<CPU::Flags::D>(false); })
    DECODE_IMP(SED, { cpu.SetFlag<CPU::Flags::D>(true); })
    DECODE_IMP(CLV, { cpu.SetFlag<CPU::Flags::V>(false); })

    DECODE_REL(BNE, ((cpu.P & CPU::Flags::Z) == 0))
    DECODE_REL(BEQ, ((cpu.P & CPU::Flags::Z) != 0))
    DECODE_REL(BCC, ((cpu.P & CPU::Flags::C) == 0))
    DECODE_REL(BCS, ((cpu.P & CPU::Flags::C) != 0))
    DECODE_REL(BPL, ((cpu.P & CPU::Flags::N) == 0))
    DECODE_REL(BMI, ((cpu.P & CPU::Flags::N) != 0))
    DECODE_REL(BVC, ((cpu.P & CPU::Flags::V) == 0))
    DECODE_REL(BVS, ((cpu.P & CPU::Flags::V) != 0))
    DECODE_ABS(JSR, NOOP, {
        cpu.PushStack((cpu.PC-1) >> 8 );
        cpu.PushStack((cpu.PC-1) & 0xff );
        cpu.PC = operand;
    })
    DECODE_ABS(JMP, NOOP, {
        cpu.PC = operand;
    })
    JMP_IND: {
        operand = bus.Read16(cpu.PC);

        if ((operand & 0xFF) == 0xFF) {
            u8 lo = bus.Read8(operand);
            u8 hi = bus.Read8(operand - 0xFF);
            operand =  (lo | hi << 8);
        } else {
            operand = bus.Read16(operand);
        }
        cpu.PC = operand;
        GOTO_NEXT(0)
    }

    // ALU OPCODES
#define ALU_OP(name, code) \
    DECODE_IMM(name, NOOP8, code) \
    DECODE_ABS(name, CHECKED_READ_8, code) \
    DECODE_ABX(name, CHECKED_READ_8, code, PAGE_CROSS_CHECK(X)) \
    DECODE_ABY(name, CHECKED_READ_8, code, PAGE_CROSS_CHECK(Y)) \
    DECODE_INX(name, CHECKED_READ_8, code) \
    DECODE_INY(name, CHECKED_READ_8, code, PAGE_CROSS_CHECK(Y)) \
    DECODE_ZPA(name, CHECKED_READ_8, code) \
    DECODE_ZPX(name, CHECKED_READ_8, code)

    ALU_OP(ORA, {
        cpu.A |= value;
        cpu.SetNZ(cpu.A);
    })
    ALU_OP(AND, {
        cpu.A &= value;
        cpu.SetNZ(cpu.A);
    })
    ALU_OP(EOR, {
        cpu.A ^= value;
        cpu.SetNZ(cpu.A);
    })
    ALU_OP(ADC, {
        u8 carry_out;
        u8 result = __builtin_addcb(cpu.A, value, cpu.P & 1, &carry_out);
        cpu.SetFlag<CPU::Flags::C>(carry_out);
//        u16 result = (u16)cpu.A + (u16)value + ((cpu.P & CPU::Flags::C) != 0);
//        cpu.SetFlag<CPU::Flags::C>(result > 0xFF);
        cpu.SetFlag<CPU::Flags::V>((~(cpu.A ^ value) & (cpu.A ^ result) & 0x80) != 0);
        cpu.SetNZ(result);
        cpu.A = (u8)result;
    })

    // sta is different from the other ALU ops
#define STA_OP(code) \
    DECODE_ABS(STA, NOOP, code) \
    DECODE_ABX(STA, NOOP, code, 0) \
    DECODE_ABY(STA, NOOP, code, 0) \
    DECODE_INX(STA, NOOP, code) \
    DECODE_INY(STA, NOOP, code, 0) \
    DECODE_ZPA(STA, NOOP, code) \
    DECODE_ZPX(STA, NOOP, code)
    STA_OP({
        CHECKED_WRITE_8(operand, cpu.A);
    })

#define STX_OP(code) \
    DECODE_ZPA(STX, NOOP, code) \
    DECODE_ZPY(STX, NOOP, code) \
    DECODE_ABS(STX, NOOP, code)
    STX_OP({
        CHECKED_WRITE_8(operand, cpu.X);
    })

#define STY_OP(code) \
    DECODE_ZPA(STY, NOOP, code) \
    DECODE_ZPX(STY, NOOP, code) \
    DECODE_ABS(STY, NOOP, code)
    STY_OP({
        CHECKED_WRITE_8(operand, cpu.Y);
    })

    ALU_OP(LDA, {
        cpu.A = value;
        cpu.SetNZ(value);
    })
    ALU_OP(CMP, {
        COMPARE_OP(cpu.A)
    })
    ALU_OP(SBC, {
//        u16 result = (u16)cpu.A + (u16)(value^0xff) + ((cpu.P & CPU::Flags::C) != 0);
//        cpu.SetFlag<CPU::Flags::C>(result > 0xFF);
        uint8_t carry_out;
        uint8_t result = __builtin_addcb(cpu.A, value ^ 0xff, cpu.P & 1, &carry_out);
        cpu.SetFlag<CPU::Flags::C>(carry_out);
        cpu.SetFlag<CPU::Flags::V>((~(cpu.A ^ value) & (cpu.A ^ result) & 0x80) != 0);
        cpu.SetNZ(result);
        cpu.A = (u8)result;
    })
#define LDX_OP(code) \
    DECODE_IMM(LDX, NOOP8, code) \
    DECODE_ABS(LDX, CHECKED_READ_8, code) \
    DECODE_ABY(LDX, CHECKED_READ_8, code, PAGE_CROSS_CHECK(Y)) \
    DECODE_ZPA(LDX, CHECKED_READ_8, code) \
    DECODE_ZPY(LDX, CHECKED_READ_8, code)
    LDX_OP({
       cpu.X = value;
       cpu.SetNZ(value);
    })

#define LDY_OP(code) \
    DECODE_IMM(LDY, NOOP8, code) \
    DECODE_ABS(LDY, CHECKED_READ_8, code) \
    DECODE_ABX(LDY, CHECKED_READ_8, code, PAGE_CROSS_CHECK(X)) \
    DECODE_ZPA(LDY, CHECKED_READ_8, code) \
    DECODE_ZPX(LDY, CHECKED_READ_8, code)
    LDY_OP({
       cpu.Y = value;
       cpu.SetNZ(value);
    })

#define CPR_OP(name, code) \
    DECODE_IMM(name, NOOP8, code) \
    DECODE_ABS(name, CHECKED_READ_8, code) \
    DECODE_ZPA(name, CHECKED_READ_8, code)
    CPR_OP(CPX, {
        COMPARE_OP(cpu.X)
    })
    CPR_OP(CPY, {
        COMPARE_OP(cpu.Y)
    })

    // RMW OPCODES

#define RMW_OP(name, code) \
DECODE_ABS(name, CHECKED_READ_8, code) \
DECODE_ABX(name, CHECKED_READ_8, code, 0) \
DECODE_ZPA(name, CHECKED_READ_8, code) \
DECODE_ZPX(name, CHECKED_READ_8, code)
    RMW_OP(ASL, {
        u16 result = (u16)value << 1;
        CHECKED_WRITE_8(operand, (u8)result);
        cpu.SetFlag<CPU::Flags::C>(result > 0xFF);
        cpu.SetNZ(result);
    })
    DECODE_IMP(ASL, {
        u16 result = (u16)cpu.A << 1;
        cpu.A = (u8)result;
        cpu.SetFlag<CPU::Flags::C>(result > 0xFF);
        cpu.SetNZ(result);
    })
    RMW_OP(ROL, {
        u16 result = ((u16)value << 1) | (cpu.P & CPU::Flags::C);
        CHECKED_WRITE_8(operand, (u8)result);
        cpu.SetFlag<CPU::Flags::C>(result > 0xFF);
        cpu.SetNZ(result);
    })
    DECODE_IMP(ROL, {
        u16 result = ((u16)cpu.A << 1) | (cpu.P & CPU::Flags::C);
        cpu.A = (u8)result;
        cpu.SetFlag<CPU::Flags::C>(result > 0xFF);
        cpu.SetNZ(result);
    })
    RMW_OP(LSR, {
        u8 result = value >> 1;
        CHECKED_WRITE_8(operand, (u8)result);
        cpu.SetFlag<CPU::Flags::C>((value & 1) == 1);
        cpu.SetNZ(result);
    })
    DECODE_IMP(LSR, {
        u8 result = cpu.A >> 1;
        cpu.SetFlag<CPU::Flags::C>((cpu.A & 1) == 1);
        cpu.A = (u8)result;
        cpu.SetNZ(result);
    })
    RMW_OP(ROR, {
        u8 result = (value >> 1) | ((cpu.P & CPU::Flags::C) << 7);
        CHECKED_WRITE_8(operand, (u8)result);
        cpu.SetFlag<CPU::Flags::C>((value & 1) == 1);
        cpu.SetNZ(result);
    })
    DECODE_IMP(ROR, {
        u8 result = (cpu.A >> 1) | ((cpu.P & CPU::Flags::C) << 7);
        cpu.SetFlag<CPU::Flags::C>((cpu.A & 1) == 1);
        cpu.A = (u8)result;
        cpu.SetNZ(result);
    })
    RMW_OP(DEC, {
        CHECKED_WRITE_8(operand, (u8)--value);
        cpu.SetNZ(value);
    })
    DECODE_IMP(DEX, {
        cpu.X--;
        cpu.SetNZ(cpu.X);
    })
    DECODE_IMP(DEY, {
        cpu.Y--;
        cpu.SetNZ(cpu.Y);
    })
    RMW_OP(INC, {
        CHECKED_WRITE_8(operand, (u8)++value);
        cpu.SetNZ(value);
    })
    DECODE_IMP(INX, {
        cpu.X++;
        cpu.SetNZ(cpu.X);
    })
    DECODE_IMP(INY, {
        cpu.Y++;
        cpu.SetNZ(cpu.Y);
    })

    // Unofficial Opcodes

#define OP_NOP() \
    DECODE_IMM(NOP, NOOP, {}) \
    DECODE_ABS(NOP, NOOP, {}) \
    DECODE_ABX(NOP, NOOP, {}, 0) \
    DECODE_ZPA(NOP, NOOP, {}) \
    DECODE_ZPX(NOP, NOOP, {})

    OP_NOP()

    // Unimplemented
    ANC_IMM:
    STP_IMP:
    XAA_IMM:
    ALR_IMM:
    ARR_IMM:
    AHX_ABY:
    AHX_INY:
    AXS_IMM:
    LAS_ABY:
    SHY_ABX:
    SHX_ABY:
    TAS_ABY:
    SAX_ZPY:
    LAX_ZPY: {
        GOTO_NEXT(0)
    }
    ALU_OP(SLO, {})
    ALU_OP(SRE, {})
    ALU_OP(LAX, {})
    ALU_OP(SAX, {})
    ALU_OP(RRA, {})
    ALU_OP(DCP, {})
    ALU_OP(ISC, {})
    ALU_OP(RLA, {})

MMIO:
    // We hit an MMIO register and the PPU isn't caught up,
    // so reverse back to the current instruction and catch up the other devices
    // and then next time it runs, we can read through to the device
    cpu.PC = prev_pc;
    SPDLOG_DEBUG("MMIO register read, resetting PC");
END:
    return current_cycles;
}
