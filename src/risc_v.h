#ifndef RISC_V_H
#define RISC_V_H

#include "common.h"

extern String mnemonics[];
extern const size_t num_mnemonics;
extern String pseudo_mnemonics[];
extern const size_t num_pseudo_mnemonics;
extern String reg_names[];
extern const size_t num_reg_names;
extern String fp_reg_names[];
extern const size_t num_fp_reg_names;

extern uint32_t opcodes[];

#define PSEUDO_LIST_32I_64I \
    X(PSEUDO_BEQZ,          "beqz",         OPERANDS_REG_OFFSET      ) \
    X(PSEUDO_BGEZ,          "bgez",         OPERANDS_REG_OFFSET      ) \
    X(PSEUDO_BGT,           "bgt",          OPERANDS_REG_REG_OFFSET  ) \
    X(PSEUDO_BGTU,          "bgtu",         OPERANDS_REG_REG_OFFSET  ) \
    X(PSEUDO_BGTZ,          "bgtz",         OPERANDS_REG_OFFSET      ) \
    X(PSEUDO_BLE,           "ble",          OPERANDS_REG_REG_OFFSET  ) \
    X(PSEUDO_BLEU,          "bleu",         OPERANDS_REG_REG_OFFSET  ) \
    X(PSEUDO_BLEZ,          "blez",         OPERANDS_REG_OFFSET      ) \
    X(PSEUDO_BLTZ,          "bltz",         OPERANDS_REG_OFFSET      ) \
    X(PSEUDO_BNEZ,          "bnez",         OPERANDS_REG_OFFSET      ) \
    X(PSEUDO_CALL,          "call",         OPERANDS_REG_SYMBOL      ) \
    X(PSEUDO_CSRR,          "csrr",         OPERANDS_REG_CSR         ) \
    X(PSEUDO_CSRC,          "csrc",         OPERANDS_CSR_REG         ) \
    X(PSEUDO_CSRCI,         "csrci",        OPERANDS_CSR_NUM         ) \
    X(PSEUDO_CSRS,          "csrs",         OPERANDS_CSR_REG         ) \
    X(PSEUDO_CSRSI,         "csrsi",        OPERANDS_CSR_NUM         ) \
    X(PSEUDO_CSRW,          "csrw",         OPERANDS_CSR_REG         ) \
    X(PSEUDO_CSRWI,         "csrwi",        OPERANDS_CSR_NUM         ) \
    X(PSEUDO_J,             "j",            OPERANDS_OFFSET          ) \
    X(PSEUDO_JR,            "jr",           OPERANDS_REG             ) \
    X(PSEUDO_LA,            "la",           OPERANDS_REG_SYMBOL      ) \
    X(PSEUDO_LI,            "li",           OPERANDS_REG_NUM         ) \
    X(PSEUDO_LLA,           "lla",          OPERANDS_REG_SYMBOL      ) \
    X(PSEUDO_MV,            "mv",           OPERANDS_REG_REG         ) \
    X(PSEUDO_NEG,           "neg",          OPERANDS_REG_REG         ) \
    X(PSEUDO_NOP,           "nop",          OPERANDS_NONE            ) \
    X(PSEUDO_NOT,           "not",          OPERANDS_REG_REG         ) \
    X(PSEUDO_RDCYCLE,       "rdcycle",      OPERANDS_NONE            ) \
    X(PSEUDO_RDINSTRET,     "rdinstret",    OPERANDS_NONE            ) \
    X(PSEUDO_RDTIME,        "rdtime",       OPERANDS_NONE            ) \
    X(PSEUDO_RET,           "ret",          OPERANDS_NONE            ) \
    X(PSEUDO_SEQZ,          "seqz",         OPERANDS_REG_REG         ) \
    X(PSEUDO_SGTZ,          "sgtz",         OPERANDS_REG_REG         ) \
    X(PSEUDO_SLTZ,          "sltz",         OPERANDS_REG_REG         ) \
    X(PSEUDO_SNEZ,          "snez",         OPERANDS_REG_REG         ) \
    X(PSEUDO_TAIL,          "tail",         OPERANDS_SYMBOL          ) \
    X(PSEUDO_LB,            "lb",           OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_LBU,           "lbu",          OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_LH,            "lh",           OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_LHU,           "lhu",          OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_LW,            "lw",           OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_SB,            "sb",           OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_SH,            "sh",           OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_SW,            "sw",           OPERANDS_REG_SYMBOL_REG  )

#define PSEUDO_LIST_32I \
    X(PSEUDO_RDCYCLEH,      "rdcycleh",     OPERANDS_NONE            ) \
    X(PSEUDO_RDINSTRETH,    "rdinstreth",   OPERANDS_NONE            ) \
    X(PSEUDO_RDTIMEH,       "rdtimeh",      OPERANDS_NONE            )

#define PSEUDO_LIST_64I \
    X(PSEUDO_NEGW,          "negw",         OPERANDS_REG_REG         ) \
    X(PSEUDO_SEXT_W,        "sext.w",       OPERANDS_REG_REG         ) \
    X(PSEUDO_LWU,           "lwu",          OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_LD,            "ld",           OPERANDS_REG_SYMBOL_REG  ) \
    X(PSEUDO_SD,            "sd",           OPERANDS_REG_SYMBOL_REG  ) \

#define PSEUDO_LIST_32F_64F \
    X(PSEUDO_FABS_S,        "fabs.s",       OPERANDS_REG_REG         ) \
    X(PSEUDO_FMV_S,         "fmv.s",        OPERANDS_REG_REG         ) \
    X(PSEUDO_FNEG_S,        "fneg.s",       OPERANDS_REG_REG         ) \
    X(PSEUDO_FRCSR,         "frcsr",        OPERANDS_REG             ) \
    X(PSEUDO_FRFLAGS,       "frflags",      OPERANDS_REG             ) \
    X(PSEUDO_FRRM,          "frrm",         OPERANDS_REG             ) \
    X(PSEUDO_FSCSR,         "fscsr",        OPERANDS_REG_REG         ) \
    X(PSEUDO_FSFLAGS,       "fsflags",      OPERANDS_REG_REG         ) \
    X(PSEUDO_FSRM,          "fsrm",         OPERANDS_REG_REG         )

#define PSEUDO_LIST_32D_64D \
    X(PSEUDO_FABS_D,        "fabs.d",       OPERANDS_REG_REG         ) \
    X(PSEUDO_FMV_D,         "fmv.d",        OPERANDS_REG_REG         ) \
    X(PSEUDO_FNEG_D,        "fneg.d",       OPERANDS_REG_REG         )

#define PSEUDO_LIST \
    PSEUDO_LIST_32I_64I \
    PSEUDO_LIST_32I \
    PSEUDO_LIST_64I

#define INST_LIST_32I \
    X(MNEM_LUI,     "lui",      FMT_U,  OPERANDS_REG_NUM,           0x00000037) \
    X(MNEM_AUIPC,   "auipc",    FMT_U,  OPERANDS_REG_NUM,           0x00000017) \
    X(MNEM_JAL,     "jal",      FMT_J,  OPERANDS_REG_OFFSET,        0x0000006f) \
    X(MNEM_JALR,    "jalr",     FMT_I,  OPERANDS_REG_NUM_REG,       0x00000067) \
    X(MNEM_BEQ,     "beq",      FMT_B,  OPERANDS_REG_REG_OFFSET,    0x00000063) \
    X(MNEM_BNE,     "bne",      FMT_B,  OPERANDS_REG_REG_OFFSET,    0x00001063) \
    X(MNEM_BLT,     "blt",      FMT_B,  OPERANDS_REG_REG_OFFSET,    0x00004063) \
    X(MNEM_BGE,     "bge",      FMT_B,  OPERANDS_REG_REG_OFFSET,    0x00005063) \
    X(MNEM_BLTU,    "bltu",     FMT_B,  OPERANDS_REG_REG_OFFSET,    0x00006063) \
    X(MNEM_BGEU,    "bgeu",     FMT_B,  OPERANDS_REG_REG_OFFSET,    0x00007063) \
    X(MNEM_LB,      "lb",       FMT_I,  OPERANDS_REG_NUM_REG,       0x00000003) \
    X(MNEM_LH,      "lh",       FMT_I,  OPERANDS_REG_NUM_REG,       0x00001003) \
    X(MNEM_LW,      "lw",       FMT_I,  OPERANDS_REG_NUM_REG,       0x00002003) \
    X(MNEM_LBU,     "lbu",      FMT_I,  OPERANDS_REG_NUM_REG,       0x00004003) \
    X(MNEM_LHU,     "lhu",      FMT_I,  OPERANDS_REG_NUM_REG,       0x00005003) \
    X(MNEM_SB,      "sb",       FMT_S,  OPERANDS_REG_NUM_REG,       0x00000023) \
    X(MNEM_SH,      "sh",       FMT_S,  OPERANDS_REG_NUM_REG,       0x00001023) \
    X(MNEM_SW,      "sw",       FMT_S,  OPERANDS_REG_NUM_REG,       0x00002023) \
    X(MNEM_ADDI,    "addi",     FMT_I,  OPERANDS_REG_REG_NUM,       0x00000013) \
    X(MNEM_SLTI,    "slti",     FMT_I,  OPERANDS_REG_REG_NUM,       0x00002013) \
    X(MNEM_SLTIU,   "sltiu",    FMT_I,  OPERANDS_REG_REG_NUM,       0x00003013) \
    X(MNEM_XORI,    "xori",     FMT_I,  OPERANDS_REG_REG_NUM,       0x00004013) \
    X(MNEM_ORI,     "ori",      FMT_I,  OPERANDS_REG_REG_NUM,       0x00006013) \
    X(MNEM_ANDI,    "andi",     FMT_I,  OPERANDS_REG_REG_NUM,       0x00007013) \
    X(MNEM_SLLI,    "slli",     FMT_I,  OPERANDS_REG_REG_NUM,       0x00001013) \
    X(MNEM_SRLI,    "srli",     FMT_I,  OPERANDS_REG_REG_NUM,       0x00005013) \
    X(MNEM_SRAI,    "srai",     FMT_I,  OPERANDS_REG_REG_NUM,       0x40005013) \
    X(MNEM_ADD,     "add",      FMT_R,  OPERANDS_REG_REG_REG,       0x00000033) \
    X(MNEM_SUB,     "sub",      FMT_R,  OPERANDS_REG_REG_REG,       0x40000033) \
    X(MNEM_SLL,     "sll",      FMT_R,  OPERANDS_REG_REG_REG,       0x00001033) \
    X(MNEM_SLT,     "slt",      FMT_R,  OPERANDS_REG_REG_REG,       0x00002033) \
    X(MNEM_SLTU,    "sltu",     FMT_R,  OPERANDS_REG_REG_REG,       0x00003033) \
    X(MNEM_XOR,     "xor",      FMT_R,  OPERANDS_REG_REG_REG,       0x00004033) \
    X(MNEM_SRL,     "srl",      FMT_R,  OPERANDS_REG_REG_REG,       0x00005033) \
    X(MNEM_SRA,     "sra",      FMT_R,  OPERANDS_REG_REG_REG,       0x40005033) \
    X(MNEM_OR,      "or",       FMT_R,  OPERANDS_REG_REG_REG,       0x00006033) \
    X(MNEM_AND,     "and",      FMT_R,  OPERANDS_REG_REG_REG,       0x00007033) \
    X(MNEM_FENCE,   "fence",    FMT_I,  OPERANDS_IORW_IORW,         0x0000000f) \
    X(MNEM_FENCE_I, "fence.i",  FMT_I,  OPERANDS_NONE,              0x0000100f) \
    X(MNEM_ECALL,   "ecall",    FMT_I,  OPERANDS_NONE,              0x00000073) \
    X(MNEM_EBREAK,  "ebreak",   FMT_I,  OPERANDS_NONE,              0x00100073) \
    X(MNEM_CSRRW,   "csrrw",    FMT_I,  OPERANDS_REG_CSR_REG,       0x00001073) \
    X(MNEM_CSRRS,   "csrrs",    FMT_I,  OPERANDS_REG_CSR_REG,       0x00002073) \
    X(MNEM_CSRRC,   "csrrc",    FMT_I,  OPERANDS_REG_CSR_REG,       0x00003073) \
    X(MNEM_CSRRWI,  "csrrwi",   FMT_I,  OPERANDS_REG_CSR_NUM,       0x00005073) \
    X(MNEM_CSRRSI,  "csrrsi",   FMT_I,  OPERANDS_REG_CSR_NUM,       0x00006073) \
    X(MNEM_CSRRCI,  "csrrci",   FMT_I,  OPERANDS_REG_CSR_NUM,       0x00007073)

#define INST_LIST_64I \
    X(MNEM_LWU,     "lwu",      FMT_I,  OPERANDS_REG_NUM_REG,       0x00006003) \
    X(MNEM_LD,      "ld",       FMT_I,  OPERANDS_REG_NUM_REG,       0x00003003) \
    X(MNEM_SD,      "sd",       FMT_S,  OPERANDS_REG_NUM_REG,       0x00003023) \
    X(MNEM_ADDIW,   "addiw",    FMT_I,  OPERANDS_REG_REG_NUM,       0x0000001b) \
    X(MNEM_SLLIW,   "slliw",    FMT_I,  OPERANDS_REG_REG_NUM,       0x0000101b) \
    X(MNEM_SRLIW,   "srliw",    FMT_I,  OPERANDS_REG_REG_NUM,       0x0000501b) \
    X(MNEM_SRAIW,   "sraiw",    FMT_I,  OPERANDS_REG_REG_NUM,       0x4000501b) \
    X(MNEM_ADDW,    "addw",     FMT_R,  OPERANDS_REG_REG_REG,       0x0000003b) \
    X(MNEM_SUBW,    "subw",     FMT_R,  OPERANDS_REG_REG_REG,       0x4000003b) \
    X(MNEM_SLLW,    "sllw",     FMT_R,  OPERANDS_REG_REG_REG,       0x0000103b) \
    X(MNEM_SRLW,    "srlw",     FMT_R,  OPERANDS_REG_REG_REG,       0x0000503b) \
    X(MNEM_SRAW,    "sraw",     FMT_R,  OPERANDS_REG_REG_REG,       0x4000503b)

#define INST_LIST_M \
    X(MNEM_MUL,     "mul",      FMT_R,  OPERANDS_REG_REG_REG,       0x02000033) \
    X(MNEM_MULH,    "mulh",     FMT_R,  OPERANDS_REG_REG_REG,       0x02001033) \
    X(MNEM_MULHSU,  "mulhsu",   FMT_R,  OPERANDS_REG_REG_REG,       0x02002033) \
    X(MNEM_MULHU,   "mulhu",    FMT_R,  OPERANDS_REG_REG_REG,       0x02003033) \
    X(MNEM_DIV,     "div",      FMT_R,  OPERANDS_REG_REG_REG,       0x02004033) \
    X(MNEM_DIVU,    "divu",     FMT_R,  OPERANDS_REG_REG_REG,       0x02005033) \
    X(MNEM_REM,     "rem",      FMT_R,  OPERANDS_REG_REG_REG,       0x02006033) \
    X(MNEM_REMU,    "remu",     FMT_R,  OPERANDS_REG_REG_REG,       0x02007033)

#define INST_LIST_IC \
    X(MNEM_C_NOP,       "c.nop",        FMT_CI,     OPERANDS_NONE,          0x0001) \
    X(MNEM_C_ADDI,      "c.addi",       FMT_CI,     OPERANDS_REG_NUM,       0x0001) \
    X(MNEM_C_JAL,       "c.jal",        FMT_CJ,     OPERANDS_OFFSET,        0x2001) \
    X(MNEM_C_LI,        "c.li",         FMT_CI,     OPERANDS_REG_NUM,       0x4001) \
    X(MNEM_C_ADDI16SP,  "c.addi16sp",   FMT_CI,     OPERANDS_NUM,           0x6101) \
    X(MNEM_C_LUI,       "c.lui",        FMT_CI,     OPERANDS_REG_NUM,       0x6001) \
    X(MNEM_C_SRLI,      "c.srli",       FMT_CI,     OPERANDS_REG_NUM,       0x8001) \
    X(MNEM_C_SRAI,      "c.srai",       FMT_CI,     OPERANDS_REG_NUM,       0x8401) \
    X(MNEM_C_ANDI,      "c.andi",       FMT_CI,     OPERANDS_REG_NUM,       0x8801) \
    X(MNEM_C_SUB,       "c.sub",        FMT_CR,     OPERANDS_REG_REG,       0x8c01) \
    X(MNEM_C_XOR,       "c.xor",        FMT_CR,     OPERANDS_REG_REG,       0x8c21) \
    X(MNEM_C_OR,        "c.or",         FMT_CR,     OPERANDS_REG_REG,       0x8c41) \
    X(MNEM_C_AND,       "c.and",        FMT_CR,     OPERANDS_REG_REG,       0x8c61) \
    X(MNEM_C_J,         "c.j",          FMT_CJ,     OPERANDS_OFFSET,        0xa001) \
    X(MNEM_C_BEQZ,      "c.beqz",       FMT_CB,     OPERANDS_REG_OFFSET,    0xc001) \
    X(MNEM_C_BNEZ,      "c.bnez",       FMT_CB,     OPERANDS_REG_OFFSET,    0xe001) \
    X(MNEM_C_ADDI4SPN,  "c.addi4spn",   FMT_CIW,    OPERANDS_REG_REG_NUM,   0x0000) \
    X(MNEM_C_LW,        "c.lw",         FMT_CL,     OPERANDS_REG_NUM_REG,   0x4000) \
    X(MNEM_C_SW,        "c.sw",         FMT_CL,     OPERANDS_REG_NUM_REG,   0xc000) \
    X(MNEM_C_SLLI,      "c.slli",       FMT_CI,     OPERANDS_REG_NUM,       0x0002) \
    X(MNEM_C_LWSP,      "c.lwsp",       FMT_CSS,    OPERANDS_REG_NUM_REG,   0x3002) \
    X(MNEM_C_JR,        "c.jr",         FMT_CSS,    OPERANDS_REG,           0x8002) \
    X(MNEM_C_MV,        "c.mv",         FMT_CR,     OPERANDS_REG_REG,       0x8002) \
    X(MNEM_C_EBREAK,    "c.ebreak",     FMT_CI,     OPERANDS_NONE,          0x9002) \
    X(MNEM_C_JALR,      "c.jalr",       FMT_CJ,     OPERANDS_REG,           0x9002) \
    X(MNEM_C_ADD,       "c.add",        FMT_CR,     OPERANDS_REG_REG,       0x9002) \
    X(MNEM_C_SWSP,      "c.swsp",       FMT_CSS,    OPERANDS_REG_NUM_REG,   0xc002)

#define INST_LIST_RV32DC \
    X(MNEM_C_FLD,       "c.fld",        FMT_CL,     OPERANDS_REG_NUM_REG,   0x2000) \
    X(MNEM_C_FSD,       "c.fsd",        FMT_CL,     OPERANDS_REG_NUM_REG,   0xa000) \
    X(MNEM_C_FLDSP,     "c.fldsp",      FMT_CI,     OPERANDS_REG_NUM_REG,   0x2002) \
    X(MNEM_C_FSDSP,     "c.fsdsp",      FMT_CSS,    OPERANDS_REG_NUM_REG,   0xa002)

#define INST_LIST_RV32FC \
    X(MNEM_C_FLW,       "c.flw",        FMT_CL,     OPERANDS_REG_NUM_REG,   0x6000) \
    X(MNEM_C_FSW,       "c.fsw",        FMT_CL,     OPERANDS_REG_NUM_REG,   0xe000) \
    X(MNEM_C_FLWSP,     "c.flwsp",      FMT_CSS,    OPERANDS_REG_NUM_REG,   0x6002) \
    X(MNEM_C_FSWSP,     "c.fswsp",      FMT_CSS,    OPERANDS_REG_NUM_REG,   0xe002)

#define INST_LIST \
    INST_LIST_32I       \
    INST_LIST_64I       \
    INST_LIST_M         \
    INST_LIST_IC        \
    INST_LIST_RV32DC    \
    INST_LIST_RV32FC

typedef enum {
#define X(MNEM, STR, OPERANDS) MNEM,
    PSEUDO_LIST
#undef X
} Pseudo;

typedef enum {
#define X(MNEM, STR, FMT, OPERANDS, OPCODE) MNEM,
    INST_LIST
#undef X
} Mnemonic;


#endif /* RISC_V_H */
