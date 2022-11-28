#include "common.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_TOKENS_PER_LINE 10
#define MAX_PSEUDO_EXPAND 3

#define EXT_M 1
#define EXT_A 2
#define EXT_F 4
#define EXT_D 8
#define EXT_C 16
int extensions = 0;

#define OPTION_RVC 1
#define OPTION_PIC 2
#define OPTION_RELAX 4
int option_stack[32] = { OPTION_RVC };
size_t option_sp = 0;

#define PSEUDO_LIST \
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
    X(PSEUDO_SUB,           "sub",          OPERANDS_REG_REG_REG     ) \
    X(PSEUDO_TAIL,          "tail",         OPERANDS_SYMBOL          ) \
    X(PSEUDO_RDCYCLEH,      "rdcycleh",     OPERANDS_NONE            ) \
    X(PSEUDO_RDINSTRETH,    "rdinstreth",   OPERANDS_NONE            ) \
    X(PSEUDO_RDTIMEH,       "rdtimeh",      OPERANDS_NONE            )

typedef enum {
#define X(MNEM, STR, OPERANDS) MNEM,
    PSEUDO_LIST
#undef X
} Pseudo;

const char * pseudo_mnemonics[] = {
#define X(MNEM, STR, OPERANDS) STR,
    PSEUDO_LIST
#undef X
    "invalid"
};

const size_t num_pseudo_mnemonics = NELEM(pseudo_mnemonics);

#define INST_LIST_I \
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
    X(MNEM_C_BEQZ,      "c.beqz",       FMT_CB,     OPERANDS_REG_NUM,       0xc001) \
    X(MNEM_C_BNEZ,      "c.bnez",       FMT_CB,     OPERANDS_REG_NUM,       0xe001) \
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
    INST_LIST_I         \
    INST_LIST_M         \
    INST_LIST_IC        \
    INST_LIST_RV32DC    \
    INST_LIST_RV32FC

typedef enum {
#define X(MNEM, STR, FMT, OPERANDS, OPCODE) MNEM,
    INST_LIST
#undef X
} Mnemonic;

const char * mnemonics[] = {
#define X(MNEM, STR, FMT, OPERANDS, OPCODE) STR,
    INST_LIST
#undef X
};

const size_t num_mnemonics = NELEM(mnemonics);

uint32_t opcodes[] = {
#define X(MNEM, STR, FMT, OPERANDS, OPCODE) OPCODE,
    INST_LIST
#undef X
};

typedef enum {
    FMT_B,
    FMT_I,
    FMT_J,
    FMT_R,
    FMT_S,
    FMT_U,
    FMT_CB,
    FMT_CI,
    FMT_CIW,
    FMT_CJ,
    FMT_CL,
    FMT_CR,
    FMT_CSS,
} Format;

Format format_of_instr[] = {
#define X(MNEM, STR, FMT, OPERANDS, OPCODE) FMT,
    INST_LIST
#undef X
};

// OFFSET: NUMBER or IDENT
typedef enum {
    OPERANDS_NONE,
    OPERANDS_IORW_IORW,              // fence [pred,succ]
    OPERANDS_REG_OFFSET,             // jal [rd,]offset
    OPERANDS_REG_NUM,                // auipc rd,imm ; lui rd,imm ; li rd,imm
    OPERANDS_REG_REG_REG,
    OPERANDS_REG_REG_OFFSET,
    OPERANDS_REG_REG_NUM,            // slli rd,rs1,shamt
    OPERANDS_REG_NUM_REG,
    OPERANDS_REG_CSR_REG,
    OPERANDS_REG_CSR_NUM,
    OPERANDS_REG,
    OPERANDS_REG_REG,
    OPERANDS_OFFSET,
    OPERANDS_NUM
} Operands;

uint32_t operands_for_mnemonic[] = {
#define X(MNEM, STR, FMT, OPERANDS, OPCODE) OPERANDS,
    INST_LIST
#undef X
};

static bool
compressed_available()
{
    return (extensions & EXT_C) && (option_stack[option_sp] & OPTION_RVC);
}

static int
reg_name_to_bits(const char * reg_name, int ln)
{
    if (strlen(reg_name) < 2)
        goto error;

    if (strcmp(reg_name, "ra") == 0) return 1;
    if (strcmp(reg_name, "sp") == 0) return 2;
    if (strcmp(reg_name, "gp") == 0) return 3;
    if (strcmp(reg_name, "tp") == 0) return 4;
    if (strcmp(reg_name, "fp") == 0) return 8;

    int n;
    if (parse_int(reg_name+1, &n) < 0)
        goto error;

    switch (reg_name[0]) {
        case 'x':
            if (n < 0 || n > 31)
                goto error;
            return n;
        case 'a':
            if (n < 0 || n > 7)
                goto error;
            return n+10;
        case 's':
            if (n < 0 || n > 31)
                goto error;
            return (n <= 1) ? n+8 : n-2+18;
        case 't':
            if (n < 0 || n > 6)
                goto error;
            return (n <= 2) ? n+5 : n-3+28;
    }

error:
    die("error: invalid register name '%s' on line %d\n", reg_name, ln);
    return 0;
}

#define FROM_MASK_TO(N, FROM, MASK, TO) (((imm >> (FROM)) & (MASK)) << (TO))

static uint32_t
b_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= FROM_MASK_TO(imm, 12,    1, 31);
    imm_fmt |= FROM_MASK_TO(imm,  5, 0x3f, 25);
    imm_fmt |= FROM_MASK_TO(imm,  1,  0xf,  8);
    imm_fmt |= FROM_MASK_TO(imm, 11,  0x1,  7);
    return imm_fmt;
}

static uint32_t
i_fmt_imm(uint32_t imm)
{
    return FROM_MASK_TO(imm, 0, 0xfff, 20);
}

static uint32_t
u_fmt_imm(uint32_t imm)
{
    return FROM_MASK_TO(imm, 0, 0xfffff, 12);
}

static uint32_t
s_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= FROM_MASK_TO(imm, 5, 0x7f, 25);
    imm_fmt |= FROM_MASK_TO(imm, 0, 0x1f,  7);
    return imm_fmt;
}

static uint32_t
j_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= FROM_MASK_TO(imm, 20,     1, 19);
    imm_fmt |= FROM_MASK_TO(imm,  1, 0x3ff,  9);
    imm_fmt |= FROM_MASK_TO(imm, 11,     1,  8);
    imm_fmt |= FROM_MASK_TO(imm, 12,  0xff,  0);
    return imm_fmt << 12;
}

static uint32_t
ci_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= FROM_MASK_TO(imm, 5,    1, 12);
    imm_fmt |= FROM_MASK_TO(imm, 0, 0x1f,  2);
    return imm_fmt;
}

static uint32_t
cj_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= FROM_MASK_TO(imm, 11, 1, 12);
    imm_fmt |= FROM_MASK_TO(imm,  4, 1, 11);
    imm_fmt |= FROM_MASK_TO(imm,  8, 3,  9);
    imm_fmt |= FROM_MASK_TO(imm, 10, 1,  8);
    imm_fmt |= FROM_MASK_TO(imm,  6, 1,  7);
    imm_fmt |= FROM_MASK_TO(imm,  7, 1,  6);
    imm_fmt |= FROM_MASK_TO(imm,  1, 7,  3);
    imm_fmt |= FROM_MASK_TO(imm,  5, 1,  2);
    return imm_fmt;
}

typedef struct {
    char * s;
    uint32_t addr;
    int ln;
} Symbol;

typedef enum {
    REF_J,
    REF_B,
    REF_CJ
} RefType;

typedef struct {
    char * s;
    RefType t;
    uint32_t addr;
    int ln;
} Ref;

Symbol * symbols;
size_t num_symbols;
size_t symbols_cap;

size_t num_refs;
size_t refs_cap;
Ref * refs;

// TODO: use this
//typedef struct {
//    int ln;
//    size_t curr_addr;
//    uint32_t * output;
//} ParserState;

static size_t
lookup_symbol(const char * s)
{
    size_t i;
    for (i = 0; i < num_symbols; i++) {
        if (strcmp(symbols[i].s, s) == 0)
            break;
    }
    return i;
}

static void
add_symbol(const char * s, uint32_t addr, int ln)
{
    size_t idx = lookup_symbol(s);
    if (idx < num_symbols)
        die("error on line %d: symbol '%s' already defined on line %d\n", ln, s, symbols[idx].ln);
    symbols[num_symbols].s = strdup(s);
    symbols[num_symbols].addr = addr;
    symbols[num_symbols].ln = ln;
    num_symbols++;
    if (num_symbols >= symbols_cap) {
        symbols_cap *= 2;
        symbols = realloc(symbols, sizeof(*symbols)*symbols_cap);
    }
}

static void
add_ref(const char * s, RefType rt, uint32_t addr, int ln)
{
    refs[num_refs].s = strdup(s);
    refs[num_refs].t = rt;
    refs[num_refs].addr = addr;
    refs[num_refs].ln = ln;
    num_refs++;
    if (num_refs >= refs_cap) {
        refs_cap *= 2;
        refs = realloc(refs, sizeof(*refs)*refs_cap);
    }
}

static void
print_refs_and_symbols()
{
    printf("\nsymbol table:\n");
    for (size_t i = 0; i < num_symbols; i++) {
        printf("\t%s: %08x\n", symbols[i].s, symbols[i].addr);
    }
    printf("\nreferences:\n");
    for (size_t i = 0; i < num_refs; i++) {
        printf("\t%s (%d): %02x\n", refs[i].s, refs[i].t, refs[i].addr);
    }
}

static void
deposit(Buffer * output, size_t addr, size_t size, uint64_t data)
{
    assert(size <= 8);
    if (size == 0)
        return;
    size_t addr_upper = (addr + size - 1) | 3;
    if (addr_upper >= output->cap) {
        output->cap = (addr_upper*2 < 1024) ? 1024 : addr_upper*2;
        output->p = realloc(output->p, output->cap);
    }
    if (addr_upper >= output->len) {
        output->len = addr_upper+1;
    }
    memset(output->p + addr, 0, addr_upper - addr + 1);
    pack_le(output->p + addr, size, data);
}

static void
resolve_refs(Buffer * output)
{
    for (size_t i = 0; i < num_refs; i++) {
        uint32_t opcode = unpack_le(output->p + refs[i].addr, sizeof(uint32_t));
        size_t j = lookup_symbol(refs[i].s);
        if (j == num_symbols)
            die("error: undefined symbol %s on line %d\n", refs[i].s, refs[i].ln);
        uint32_t offset = symbols[j].addr - refs[i].addr;

        if (refs[i].t == REF_J) {
            opcode |= j_fmt_imm(offset);
        } else if (refs[i].t == REF_B) {
            opcode |= b_fmt_imm(offset);
        } else if (refs[i].t == REF_CJ) {
            uint32_t imm_fmt = cj_fmt_imm(offset);
            if (refs[i].addr % 4 == 2)
                imm_fmt <<= 16;
            opcode |= imm_fmt;
        } else {
            assert(0);
        }
        deposit(output, refs[i].addr, 4, opcode);
    }
}

static bool
tokens_match(Token * tokens, size_t num_tokens, const char * fmt)
{
    size_t fmt_l = strlen(fmt);
    size_t ti = 0;
    for (size_t i = 0; i < fmt_l; i++) {
        TokenType tt;
        bool special = false;
        switch (fmt[i]) {
            case ' ': continue;
            case ',': tt = ',';         break;
            case ':': tt = ':';         break;
            case '(': tt = '(';         break;
            case ')': tt = ')';         break;
            case '\n': tt = '\n';       break;
            case 'd': tt = TOK_DIR;     break;
            case 'm': tt = TOK_MNEM;    break;
            case 'r': tt = TOK_REG;     break;
            case 'c': tt = TOK_CSR;     break;
            case 'n': tt = TOK_NUM;     break;
            case 'i': tt = TOK_IDENT;   break;
            case 's': tt = TOK_STRING;  break;
            case 'o': special = true;   break;
            default: assert(0);
        }
        if (ti >= num_tokens)
            return false;
        TokenType curr_token = tokens[ti++].t;
        if (special) {
            if (fmt[i] == 'o') {
                if ((curr_token != TOK_NUM) &&
                    (curr_token != TOK_IDENT))
                    return false;
            }
        } else {
            if (curr_token != tt)
                return false;
        }
    }
    return true;
}

static void
tokens_match_or_die(Token * tokens, size_t num_tokens, const char * fmt, int ln)
{
    if (!tokens_match(tokens, num_tokens, fmt))
        die("error: parse error on line %d\n", ln);
}

#if 0
static bool
tokens_match(Token * tokens, size_t num_tokens, size_t n, ...)
{
    va_list ap;
    if (num_tokens != n)
        return false;
    va_start(ap, n);
    for (size_t i = 0; i < n; i++) {
        if (tokens[i].t != va_arg(ap, TokenType))
            return false;
    }
	va_end(ap);
    return true;
}
#endif

static int
parse_int_or_die(const char * s, int ln)
{
    int i;
    int errnum = parse_int(s, &i);
    if (errnum < 0)
        die("error: %s on line %d\n", parse_int_strerror(errnum), ln);
    return i;
}

static int
parse_iorw_or_die(const char * s, int ln)
{
    int iorw = 0;
    char c;
    while ((c = *s++)) {
        if (c == 'i')
            iorw |= 8;
        else if (c == 'o')
            iorw |= 4;
        else if (c == 'r')
            iorw |= 2;
        else if (c == 'w')
            iorw |= 1;
        else
            die("error: invalid pred/succ on line %d\n", ln);
    }
    return iorw;
}

static size_t
parse_instr(Token * tokens, size_t num_tokens, Buffer * output, size_t curr_addr, int ln)
{
    Mnemonic mnemonic = str_idx_in_list(tokens[0].s, mnemonics, num_mnemonics);
    if (mnemonic == num_mnemonics)
        die("error: invalid mnemonic '%s' on line %d\n", tokens[0].s, ln);
    bool compressed = strncmp(tokens[0].s, "c.", 2) == 0;
    uint32_t opcode = opcodes[mnemonic];
    Operands expected_operands = operands_for_mnemonic[mnemonic];
    int32_t imm;
    uint32_t rd, rs1, rs2;
    uint32_t imm_fmt, pred, succ;
    uint32_t csr;

    switch (expected_operands) {
        case OPERANDS_NONE:
            tokens_match_or_die(tokens, num_tokens, "", ln);
            break;

        case OPERANDS_IORW_IORW:
            if (num_tokens == 4) {
                tokens_match_or_die(tokens, num_tokens, "m i,i", ln);
                pred = parse_iorw_or_die(tokens[1].s, ln);
                succ = parse_iorw_or_die(tokens[3].s, ln);
            } else {
                tokens_match_or_die(tokens, num_tokens, "m", ln);
                pred = 15;
                succ = 15;
            }
            opcode |= (pred << 24) | (succ << 20);
            break;

        case OPERANDS_REG_OFFSET:
            imm = 0;
            if (num_tokens == 4) {
                tokens_match_or_die(tokens, num_tokens, "m r,o", ln);
                rd = reg_name_to_bits(tokens[1].s, ln);
                if (tokens[3].t == TOK_NUM)
                    imm = parse_int_or_die(tokens[3].s, ln);
                else if (tokens[3].t == TOK_IDENT)
                    add_ref(tokens[3].s, REF_J, curr_addr, ln);
                else
                    die("error: invalid format for %s instruction on line %d\n", tokens[0].s, ln);
            } else {
                tokens_match_or_die(tokens, num_tokens, "m o", ln);
                rd = reg_name_to_bits("x1", ln);
                if (tokens[1].t == TOK_NUM)
                    imm = parse_int_or_die(tokens[1].s, ln);
                else if (tokens[1].t == TOK_IDENT)
                    add_ref(tokens[1].s, REF_J, curr_addr, ln);
                else
                    die("error: invalid format for %s instruction on line %d\n", tokens[0].s, ln);
            }
            opcode |= j_fmt_imm(imm) | (rd << 7);
            break;

        case OPERANDS_REG_NUM:
            tokens_match_or_die(tokens, num_tokens, "m r,n", ln);
            rd  = reg_name_to_bits(tokens[1].s, ln);
            imm = parse_int_or_die(tokens[3].s, ln);
            if ((imm < 0) || (imm > 0xfffff))
                die("error: lui immediate must be in range 0..1048575 on line %d\n", ln);
            imm_fmt = compressed ? ci_fmt_imm(imm) : u_fmt_imm(imm);
            opcode |= imm_fmt | (rd << 7);
            break;

        case OPERANDS_REG_REG_REG:
            tokens_match_or_die(tokens, num_tokens, "m r,r,r", ln);
            rd  = reg_name_to_bits(tokens[1].s, ln);
            rs1 = reg_name_to_bits(tokens[3].s, ln);
            rs2 = reg_name_to_bits(tokens[5].s, ln);
            opcode |= (rs2 << 20) | (rs1 << 15) | (rd << 7);
            break;

        case OPERANDS_REG_REG_OFFSET:
            tokens_match_or_die(tokens, num_tokens, "m r,r,o", ln);
            rs1 = reg_name_to_bits(tokens[1].s, ln);
            rs2 = reg_name_to_bits(tokens[3].s, ln);
            if (tokens[5].t == TOK_NUM)
                imm = parse_int_or_die(tokens[5].s, ln);
            else {
                add_ref(tokens[5].s, REF_B, curr_addr, ln);
                imm = 0;
            }
            imm_fmt = b_fmt_imm(imm);
            opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
            break;

        case OPERANDS_REG_REG_NUM:
            tokens_match_or_die(tokens, num_tokens, "m r,r,n", ln);
            rd = reg_name_to_bits(tokens[1].s, ln);
            rs1 = reg_name_to_bits(tokens[3].s, ln);
            imm = parse_int_or_die(tokens[5].s, ln);
            if (mnemonic == MNEM_SLLI ||
                mnemonic == MNEM_SRLI ||
                mnemonic == MNEM_SRAI) {
                // TODO: RV32 vs RV64
                if ((imm < 0) || (imm >= 1<<5)) {
                    die("error: invalid shift amount on line %d\n", ln);
                }
            }
            opcode |= (imm << 20) | (rs1 << 15) | (rd << 7);
            // NOTE: c.addi4spn is here to match gcc
            if (strcmp(tokens[0].s, "c.addi4spn") == 0) {
                if (rs1 != 2)
                    die("error: register source must be sp for %s on line %d\n", tokens[0].s, ln);
            }
            break;

        case OPERANDS_REG_NUM_REG:
            tokens_match_or_die(tokens, num_tokens, "m r,n(r)", ln);
            imm = parse_int_or_die(tokens[3].s, ln);
            rs1 = reg_name_to_bits(tokens[5].s, ln);
            if (format_of_instr[mnemonic] == FMT_S) {
                rs2 = reg_name_to_bits(tokens[1].s, ln);
                imm_fmt = s_fmt_imm(imm);
                opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
            } else {
                rd = reg_name_to_bits(tokens[1].s, ln);
                imm_fmt = i_fmt_imm(imm);
                opcode |= imm_fmt | (rs1 << 15) | (rd << 7);
            }
            break;

        case OPERANDS_REG_CSR_REG:
            tokens_match_or_die(tokens, num_tokens, "m r,c,r", ln);
            /* TODO */
            break;

        case OPERANDS_REG_CSR_NUM:
            /* TODO */
            tokens_match_or_die(tokens, num_tokens, "m r,c,n", ln);
            rd = reg_name_to_bits(tokens[1].s, ln);
            csr = 0; // csr_name_to_bits(tokens[3].s, ln);
            imm = parse_int_or_die(tokens[5].s, ln);
            if (imm < 0 || imm >= 1<<5) {
                die("error: csr*i immediate must be in range 0..31 on line %d\n", ln);
            }
            opcode |= (csr << 20) | (imm << 15) | (rd << 7);
            break;

        case OPERANDS_OFFSET:
            tokens_match_or_die(tokens, num_tokens, "m o", ln);
            imm = 0;
            if (tokens[1].t == TOK_NUM)
                imm = parse_int_or_die(tokens[1].s, ln);
            else
                add_ref(tokens[1].s, REF_CJ, curr_addr, ln);
            assert(compressed);
            opcode |= cj_fmt_imm(imm);
            break;

        // c.add, c.and, c.or, c.mv
        case OPERANDS_REG_REG:
            tokens_match_or_die(tokens, num_tokens, "m r,r", ln);
            assert(compressed);
            // TODO: make sure rd and rs2 are correct
            rd = reg_name_to_bits(tokens[1].s, ln);
            rs2 = reg_name_to_bits(tokens[3].s, ln);
            if (mnemonic == MNEM_C_ADD) {
                if ((rd == 0) || (rs2 == 0))
                    die("error: rd and rs2 must not be 0 for %s instruction on line %d\n", tokens[0].s, ln);
            } else if (mnemonic == MNEM_C_MV) {
                if (rs2 == 0)
                    die("error: rs2 must not be 0 for %s instruction on line %d\n", tokens[0].s, ln);
            } else if ((mnemonic == MNEM_C_AND) || (mnemonic == MNEM_C_OR)) {
                // TODO: check register ranges
            }
            opcode |= (rd << 7) | (rs2 << 2);
            break;

        case OPERANDS_REG:
            tokens_match_or_die(tokens, num_tokens, "m r", ln);
            assert(compressed);
            rs1 = reg_name_to_bits(tokens[1].s, ln);
            opcode |= (rs1 << 7);
            break;


        case OPERANDS_NUM:
            tokens_match_or_die(tokens, num_tokens, "m r,n", ln); // NOTE: match gcc
            rd = reg_name_to_bits(tokens[1].s, ln);
            if (rd != 2)
                die("error: register destination must be sp for %s on line %d\n", tokens[0].s, ln);
            break;

        default:
            die("don't know how to parse %s on line %d\n", tokens[0].s, ln);
            assert(0);
            break;
    }

    size_t opcode_size = compressed ? 2: 4;
    deposit(output, curr_addr, opcode_size, opcode);
    //printf("deposit %08x (%zu) %s\n", opcode, opcode_size, mnemonics[mnemonic]);
    curr_addr += opcode_size;
    return curr_addr;
}

static size_t
align_output(Buffer * output, int alignment, size_t curr_addr)
{
    assert(alignment >= 2);
    assert((alignment & (alignment - 1)) == 0); // power of 2
    // align to 2-byte boundary
    if (curr_addr & 1)
        deposit(output, curr_addr++, 1, 0);
    if (alignment == 2)
        return curr_addr;
    // align to 4-byte boundary
    if (curr_addr % 4 == 2) {
        deposit(output, curr_addr, 2, 0x0001); // c.nop
        curr_addr += 2;
    }
    assert(curr_addr % 4 == 0);
    // insert nops until desired alignment is obtained
    while (curr_addr % alignment != 0) {
        if (compressed_available()) {
            deposit(output, curr_addr, 2, 0x0001); // c.nop
            curr_addr += 2;
        } else {
            deposit(output, curr_addr, 4, 0x00000013); // addi x0, x0, 0
            curr_addr += 4;
        }
    }
    return curr_addr;
}

static size_t
parse_directive(Token * tokens, size_t num_tokens, Buffer * output, size_t curr_addr, int ln)
{
    assert(tokens[0].t == TOK_DIR);
    // TODO: allow for lists of numbers
    // TODO: bounds checks
    if (strcmp(tokens[0].s, ".byte") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d n", ln);
        int n = parse_int_or_die(tokens[1].s, ln);
        deposit(output, curr_addr++, 1, n);
    } else if (strcmp(tokens[0].s, ".half") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d n", ln);
        int n = parse_int_or_die(tokens[1].s, ln);
        deposit(output, curr_addr, 2, n);
        curr_addr += 2;
    } else if (strcmp(tokens[0].s, ".word") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d n", ln);
        int n = parse_int_or_die(tokens[1].s, ln);
        deposit(output, curr_addr, 4, n);
        curr_addr += 4;
    } else if (strcmp(tokens[0].s, ".dword") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d n", ln);
        int64_t n = strtoll(tokens[1].s, NULL, 0);
        deposit(output, curr_addr, 8, n);
        curr_addr += 8;
    } else if (strcmp(tokens[0].s, ".string") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d s", ln);
        for (const char * cp = tokens[1].s+1; *cp != '"'; cp++) {
            deposit(output, curr_addr++, 1, *cp);
        }
        deposit(output, curr_addr++, 1, '\0');
    } else if (strcmp(tokens[0].s, ".align") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d n", ln);
        int log2_alignment = parse_int_or_die(tokens[1].s, ln);
        if ((log2_alignment < 1) || (log2_alignment > 20))
            die("error: invalid alignment %d on line %d\n", log2_alignment, ln);
        curr_addr = align_output(output, 1 << log2_alignment, curr_addr);
    } else if (strcmp(tokens[0].s, ".globl") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d i", ln);
        // TODO
    } else if (strcmp(tokens[0].s, ".text") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d", ln);
        // TODO
    } else if (strcmp(tokens[0].s, ".data") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d", ln);
        // TODO
    } else if (strcmp(tokens[0].s, ".option") == 0) {
        tokens_match_or_die(tokens, num_tokens, "d i", ln);
        if (strcmp(tokens[1].s, "push") == 0) {
            option_sp++;
            if (option_sp >= NELEM(option_stack))
                die("error: exceeded max depth of option stack on line %d\n", ln);
            option_stack[option_sp] = option_stack[option_sp-1];
        } else if (strcmp(tokens[1].s, "pop") == 0) {
            if (option_sp == 0)
                die("error: popped option stack too many times on line %d\n", ln);
            option_sp--;
        } else if (strcmp(tokens[1].s, "rvc") == 0) {
            option_stack[option_sp] |= OPTION_RVC;
        } else if (strcmp(tokens[1].s, "norvc") == 0) {
            option_stack[option_sp] &= ~OPTION_RVC;
        } else if (strcmp(tokens[1].s, "pic") == 0) {
            option_stack[option_sp] |= OPTION_PIC;
        } else if (strcmp(tokens[1].s, "nopic") == 0) {
            option_stack[option_sp] &= ~OPTION_PIC;
        } else if (strcmp(tokens[1].s, "relax") == 0) {
            option_stack[option_sp] |= OPTION_RELAX;
        } else if (strcmp(tokens[1].s, "norelax") == 0) {
            option_stack[option_sp] &= ~OPTION_RELAX;
        }
    } else {
        die("unsupported directive %s on line %d\n", tokens[0].s, ln);
    }
    return curr_addr;
}

static size_t
substitute_pseudoinstr(Token expanded_tokens[][MAX_TOKENS_PER_LINE], size_t num_expanded_tokens[], Token * tokens, size_t num_tokens, int ln)
{
    if (tokens[0].t != TOK_PSEUDO) {
        for (size_t i = 0; i < num_tokens; i++)
            expanded_tokens[0][i] = tokens[i];
        num_expanded_tokens[0] = num_tokens;
        return 1;
    }

    Pseudo pseudo = str_idx_in_list(tokens[0].s, pseudo_mnemonics, num_pseudo_mnemonics);
    switch (pseudo) {
        case PSEUDO_BEQZ:
            // beqz rs1, offset
            // beq  rs1, x0, offset
            assert(0); // TODO
        case PSEUDO_BGEZ:
            // bgez        rs1, offset
            // bge rs1, x0, offset
            assert(0); // TODO
        case PSEUDO_BGT:
            // bgt  rs1, rs2, offset
            // blt  rs2, rs1, offset
            assert(0); // TODO
        case PSEUDO_BGTU:
            // bgtu rs1, rs2, offset
            // bltu rs2, rs1, offset
            assert(0); // TODO
        case PSEUDO_BGTZ:
            // bgtz rs2, offset
            // blt  x0, rs2, offset
            assert(0); // TODO
        case PSEUDO_BLE:
            // ble  rs1, rs2, offset
            // bge  rs2, rs1, offset
            assert(0); // TODO
        case PSEUDO_BLEU:
            // bleu  rs1, rs2, offset
            // bgeu  rs2, rs1, offset
            assert(0); // TODO
        case PSEUDO_BLEZ:
            // blez rs2, offset
            // bge  x0, rs2, offset
            assert(0); // TODO
        case PSEUDO_BLTZ:
            // bltz rs1, offset
            // blt  rs1, x0, offset
            assert(0); // TODO
        case PSEUDO_BNEZ:
            // bnez rs1, offset
            // bne  rs1, x0, offset
            assert(0); // TODO
        case PSEUDO_CALL:
            // call     rd, symbol
            // auipc    rd, offsetHi; jalr rd, offsetLo(rd); if rd is omitted, x1
            assert(0); // TODO
        case PSEUDO_CSRR:
            // csrr     rd, csr
            // csrrs    rd, csr, x0
            assert(0); // TODO
        case PSEUDO_CSRC:
            // csrc     csr, rs1
            // csrrc    x0, csr, rs1
            assert(0); // TODO
        case PSEUDO_CSRCI:
            // csrci    csr, zimm[4:0]
            // csrrci   x0, csr, zimm
            assert(0); // TODO
        case PSEUDO_CSRS:
            // csrs     csr, rs1
            // csrrs    x0, csr, rs1
            assert(0); // TODO
        case PSEUDO_CSRSI:
            // csrsi    csr, zimm[4:0]
            // csrrsi   x0, csr, zimm
            assert(0); // TODO
        case PSEUDO_CSRW:
            // csrw     csr, rs1
            // csrrw    x0, csr, rs1
            assert(0); // TODO
        case PSEUDO_CSRWI:
            // csrwi    csr, zimm[4:0]
            // csrrwi   x0, csr, zimm
            assert(0); // TODO
        case PSEUDO_J:
            // j    offset
            // jal  x0, offset
            assert(0); // TODO
        case PSEUDO_JR:
            // jr   rs1
            // jalr x0, 0(rs1)
            assert(0); // TODO
        case PSEUDO_LA:
            // la       rd, symbol
            // RV32I: auipc rd, offsetHi; lw rd, offsetLo(rd)
            assert(0); // TODO
        case PSEUDO_LI:
            // li       rd, imm
            // RV32I: lui and/or addi
            assert(0); // TODO
        case PSEUDO_LLA:
            // lla      rd, symbol
            // auipc    rd, offsetHi; addi rd, rd, offsetLo
            assert(0); // TODO
        case PSEUDO_MV:
            // mv   rd, rs1
            // addi rd, rs1, 0
            assert(0); // TODO
        case PSEUDO_NEG:
            // neg  rd, rs2
            // sub  rd, x0, rs2
            assert(0); // TODO
        case PSEUDO_NOP:
            // nop
            // addi x0, x0, 0
            assert(0); // TODO
        case PSEUDO_NOT:
            // not  rd, rs1
            // xori rd, rs1, -1
            expanded_tokens[0][0] = (Token) {.t=TOK_MNEM, .s="xori"};
            expanded_tokens[0][1] = tokens[1];
            expanded_tokens[0][2] = tokens[2];
            expanded_tokens[0][3] = tokens[3];
            expanded_tokens[0][4] = (Token) {.t=',', .s=","};
            expanded_tokens[0][5] = (Token) {.t=TOK_NUM, .s="-1"};
            num_expanded_tokens[0] = 6;
            return 1;
        case PSEUDO_RDCYCLE:
            // rdcycle
            // csrrs rd, cycle, x0
            assert(0); // TODO
        case PSEUDO_RDINSTRET:
            // rdinstret
            // csrrs rd, instret, x0
            assert(0); // TODO
        case PSEUDO_RDTIME:
            // rdtime
            // csrrs rd, time, x0
            assert(0); // TODO
        case PSEUDO_RET:
            // ret
            // jalr x0, 0(x1)
            assert(0); // TODO
        case PSEUDO_SEQZ:
            // seqz     rd, rs1
            // sltiu    rd, rs1, 1
            assert(0); // TODO
        case PSEUDO_SGTZ:
            // sgtz rd, rs2
            // slt  rd, x0, rs2
            assert(0); // TODO
        case PSEUDO_SLTZ:
            // sltz rd, rs1
            // slt  rd, rs1, x0
            assert(0); // TODO
        case PSEUDO_SNEZ:
            // snez rd, rs2
            // sltu rd, x0, rs2
            assert(0); // TODO
        case PSEUDO_SUB:
            // sub      rd, rs1, rs2
            // c.sub    rd, rs2
            assert(0); // TODO
        case PSEUDO_TAIL:
            // tail     symbol
            // auipc    x6, offsetHi; jalr x0, offsetLo(x6)
            assert(0); // TODO
        case PSEUDO_RDCYCLEH:
            // rdcycleh
            // csrrs rd, cycleh, x0
            assert(0); // TODO
        case PSEUDO_RDINSTRETH:
            // rdinstreth
            // csrrs rd, instreth, x0
            assert(0); // TODO
        case PSEUDO_RDTIMEH:
            // rdtimeh
            // csrrs rd, timeh, x0
            assert(0); // TODO
        default:
            die("unsupported pseudoinstruction '%s' on line %d\n", tokens[0].s, ln);
    }
    return 0;
}

static size_t
compress_if_possible(Token * tokens, size_t num_tokens, int ln)
{
    Mnemonic mnemonic = str_idx_in_list(tokens[0].s, mnemonics, num_mnemonics);
    assert(mnemonic < num_mnemonics);
    if (mnemonic == MNEM_ADD) {
        // add      rd, rs1, rs2
        // - c.add  rd, rs2         when rd=rs1 (invalid if rd=x0 or rs2=x0)
        // - c.mv   rd, rs2         when rd=x0  (invalid if rs2=x0)
        int rd = reg_name_to_bits(tokens[1].s, ln);
        int rs1 = reg_name_to_bits(tokens[3].s, ln);
        int rs2 = reg_name_to_bits(tokens[5].s, ln);
        if ((rd == rs1) && (rd != 0) && (rs2 != 0)) {
            strcpy(tokens[0].s, "c.add");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        } else if (rs1 == 0 && rs2 != 0) {
            strcpy(tokens[0].s, "c.mv");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_ADDI) {
        // addi         rd, rs1, imm
        // - c.li       rd, imm     when rs1=x0
        // - c.addi     rd, imm     when rd=rs1
        // - c.addi16sp imm         when rd=rs1=x2 (invalid if imm=0)
        // - c.addi4spn rd-8, uimm  when rs1=x2    (invalid if uimm=0)
        // NOTE: addi can also be compressed to c.mv when imm=0
        int rd = reg_name_to_bits(tokens[1].s, ln);
        int rs1 = reg_name_to_bits(tokens[3].s, ln);
        int imm = parse_int_or_die(tokens[5].s, ln);
        // TODO: confirm bounds check on imm
        if ((rs1 == 0) && (imm < 64)) {
            strcpy(tokens[0].s, "c.li");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        } else if ((rd == rs1) && (imm < 64)) {
            strcpy(tokens[0].s, "c.addi");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        } else if ((rd == 2) && (rs1 == 2) && (imm != 0)) {
            strcpy(tokens[0].s, "c.addi16sp");
            tokens[1] = tokens[5];
            num_tokens -= 4;
        } else if ((rs1 == 2) && (imm != 0) && (rd >= 8)) {
            strcpy(tokens[0].s, "c.addi4spn");
            sprintf(tokens[1].s, "x%d", rd-8);
            // NOTE: match gcc
            //tokens[3] = tokens[5];
            //num_tokens -= 2;
        } else if (imm == 0) {
            strcpy(tokens[0].s, "c.mv");
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_AND) {
        // and      rd, rs1, rs2
        // - c.and  rd, rs2         when rd=rs1
        int rd = reg_name_to_bits(tokens[1].s, ln);
        int rs1 = reg_name_to_bits(tokens[3].s, ln);
        if ((rd == rs1) && (rd >= 8)) {
            strcpy(tokens[0].s, "c.and");
            sprintf(tokens[1].s, "x%d", rd-8);
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_ANDI) {
        // andi     rd, rs1, imm
        // - c.andi rd, imm       when rd=rs1
        int rd = reg_name_to_bits(tokens[1].s, ln);
        int rs1 = reg_name_to_bits(tokens[3].s, ln);
        if ((rd == rs1) && (rd >= 8)) {
            strcpy(tokens[0].s, "c.andi");
            sprintf(tokens[1].s, "x%d", rd-8);
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_BEQ) {
        // beq      rs1, rs2, offset
        // - c.beqz rs1, offset     when rs2=x0
        int rs2 = reg_name_to_bits(tokens[3].s, ln);
        if (rs2 == 0) {
            strcpy(tokens[0].s, "c.beqz");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_EBREAK) {
        // c.ebreak
        strcpy(tokens[0].s, "c.ebreak");
    } else if (mnemonic == MNEM_JAL) {
        // jal      rd, offset      ; if rd is omitted, x1
        // - c.j    offset          when rd=x0
        // - c.jal  offset          when rd=x1
        int rd = reg_name_to_bits(tokens[1].s, ln);
        if (rd == 0) {
            strcpy(tokens[0].s, "c.j");
            tokens[1] = tokens[3];
            num_tokens -= 2;
        } else if (rd == 1) {
            strcpy(tokens[0].s, "c.jal");
            tokens[1] = tokens[3];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_JALR) {
        // jalr     rd, offset(rs1) ; if rd is omitted, x1
        // - c.jr   rs1             when rd=x0 and offset=0
        // - c.jalr rs1             when rd=x1 and offset=0
        int offset = parse_int_or_die(tokens[3].s, ln);
        if (offset == 0) {
            int rd = reg_name_to_bits(tokens[1].s, ln);
            if (rd == 0) {
                strcpy(tokens[0].s, "c.jr");
                tokens[1] = tokens[5];
                num_tokens -= 5;
            } else if (rd == 1) {
                strcpy(tokens[0].s, "c.jalr");
                tokens[1] = tokens[5];
                num_tokens -= 5;
            }
        }
    } else if (mnemonic == MNEM_LUI) {
        // lui      rd, imm
        // - c.lui  rd, imm     when -32 <= sext(imm) < 32
        int imm = parse_int_or_die(tokens[3].s, ln);
        if ((imm < 0) || (imm > 0xfffff))
            die("error: lui immediate must be in range 0..1048575 on line %d\n", ln);
        int imm_sext = sext(imm, 20);
        if (imm_sext >= -32 && imm_sext < 32) {
            strcpy(tokens[0].s, "c.lui");
        }
        // TODO
    } else if (mnemonic == MNEM_SW) {
        // sw       rs2, offset(rs1)
        // - c.swsp rs2, offset
        // - c.sw   rs2, offset(rs1)
        // TODO
    } else if (mnemonic == MNEM_SLLI) {
        // slli     rd, rs1, shamt
        // - c.slli rd, shamt
        // TODO
    } else if (mnemonic == MNEM_SRAI) {
        // srai     rd, rs1, shamt
        // - c.srai rd, shamt
        // TODO
    } else if (mnemonic == MNEM_SRLI) {
        // srli     rd, rs1, shamt
        // - c.srli rd, shamt
        // TODO
    } else if (mnemonic == MNEM_XOR) {
        // xor      rd, rs1, rs2
        // - c.xor  rd, rs2
        // TODO
    }
    return num_tokens;
}

static size_t
get_line_of_tokens(TokenizerState * ts, Token * tokens)
{
    size_t num_tokens = 0;
    TokenType tt = TOK_NONE;
    while (tt != '\n') {
        if (num_tokens >= MAX_TOKENS_PER_LINE)
            die("error: too many tokens on line %d\n", ts->ln);
        tt = (tokens[num_tokens++] = get_token(ts)).t;
        if (tt == TOK_EOF) {
            ts->eof = 1;
            break;
        }
    }
    num_tokens--; // drop newline token
    return num_tokens;
}

/* TODO: separate parsing from outputting */
static void
parse(Buffer input)
{
    // TODO: should be a dict
    num_symbols = 0;
    symbols_cap = 10;
    symbols = malloc(sizeof(*symbols)*symbols_cap);

    num_refs = 0;
    refs_cap = 10;
    refs = malloc(sizeof(*refs)*refs_cap);

    Buffer output = {
        .len = 0,
        .cap = 1024,
    };
    output.p = malloc(output.cap);

    uint32_t curr_addr = 0;
    Token tokens[MAX_TOKENS_PER_LINE];
    Token expanded_tokens[MAX_PSEUDO_EXPAND][MAX_TOKENS_PER_LINE];
    size_t num_expanded_tokens[MAX_PSEUDO_EXPAND];
    TokenizerState ts = init_tokenizer(input);
    while (!ts.eof) {
        size_t num_tokens = get_line_of_tokens(&ts, tokens);
        if (num_tokens == 0)
            continue;

        // labels
        Token * tokens_no_label = tokens;
        if (num_tokens >= 2 && tokens[1].t == ':') {
            add_symbol(tokens[0].s, curr_addr, ts.ln);
            tokens_no_label = &tokens[2];
            num_tokens -= 2;
        }
        if (num_tokens == 0)
            continue;

        size_t num_pseudo_expansions = substitute_pseudoinstr(expanded_tokens, num_expanded_tokens, tokens_no_label, num_tokens, ts.ln);

        for (size_t r = 0; r < num_pseudo_expansions; r++) {
            if (expanded_tokens[r][0].t == TOK_MNEM) {
                if (compressed_available())
                    num_expanded_tokens[r] = compress_if_possible(expanded_tokens[r], num_expanded_tokens[r], ts.ln);
                curr_addr = parse_instr(expanded_tokens[r], num_expanded_tokens[r], &output, curr_addr, ts.ln);
            } else if (expanded_tokens[r][0].t == TOK_DIR) {
                curr_addr = parse_directive(expanded_tokens[r], num_expanded_tokens[r], &output, curr_addr, ts.ln);
            } else {
                die("error: cannot parse line %d\n", ts.ln);
            }
        }
    }

    resolve_refs(&output);

#if 0
    for (size_t i = 0; i < output.len; i++) {
        printf("%02x ", (int) unpack_le(output.p + i, 1));
        if ((i+1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
#endif

#if 0
    for (size_t i = 0; i < (curr_addr+3)/4; i++) {
        //printf("%02zx: %08x\n", i*4, (int) unpack_le(output.p + i*4, 4));
        printf("%08x\n", (int) unpack_le(output.p + i*4, 4));
    }
#else
    for (size_t i = 0; i < (curr_addr+1)/2; i++) {
        int b2 = (int) unpack_le(output.p + i*2, 2);
        bool compressed = (b2 & 3) != 3;
        if (compressed) {
            printf("%04x\n", b2);
        } else {
            int b4 = (int) unpack_le(output.p + i*2, 4);
            printf("%08x\n", b4);
            i++;
        }
    }
#endif

    //print_refs_and_symbols();

    for(size_t i = 0; i < num_symbols; i++)
        free(symbols[i].s);
    free(symbols);
    for(size_t i = 0; i < num_refs; i++)
        free(refs[i].s);
    free(refs);
}

static void
strip_comments(char * s, size_t l)
{
    /* replace comments with ' ' */
    size_t i;
    bool comment_flag = 0;
    for (i = 0; i < l; i++) {
        if (!comment_flag) {
            if (s[i] == '#') {
                s[i] = ' ';
                comment_flag = 1;
            }
        } else {
            if (s[i] != '\n')
                s[i] = ' ';
            else
                comment_flag = 0;
        }
    }
}

static void
parse_isa_string(const char * isa)
{
    if (strncmp(isa, "rv", 2) != 0) {
        die("error: unsupported ISA string '%s'\n", isa);
    }
    if ((strncmp(isa+2, "32", 2) != 0) && (strncmp(isa+2, "64", 2) != 0)) {
        die("error: unsupported ISA string '%s'\n", isa);
    }
    if (strncmp(isa+2, "64", 2) == 0) {
        die("error: RV64 is not yet supported\n", isa);
    }
    size_t isa_len = strlen(isa);
    for (size_t i = 4; i < isa_len; i++) {
        switch (isa[i]) {
            case 'i': break;
            case 'm': extensions |= EXT_M; break;
            case 'a': extensions |= EXT_A; break;
            case 'f': extensions |= EXT_F; break;
            case 'd': extensions |= EXT_D; break;
            case 'c': extensions |= EXT_C; break;
            default:
                die("error: unsupported extension '%c' in isa string '%s'\n", isa[i], isa);
        }
    }
    if (extensions & EXT_M) {
        die("error: 'm' extension not supported\n");
    }
    if (extensions & EXT_A) {
        die("error: 'a' extension not supported\n");
    }
    if (extensions & EXT_F) {
        die("error: 'f' extension not supported\n");
    }
    if (extensions & EXT_D) {
        die("error: 'd' extension not supported\n");
    }
}

int
main(int argc, char * argv[])
{
    const char * filename = NULL;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-march=", 7) == 0) {
            parse_isa_string(argv[i]+7);
        } else {
            // TODO: allow for multiple input files
            if (filename == NULL)
                filename = argv[i];
            else
                die("error: cannot specify multiple input files\n");
        }
    }

    if (filename == NULL) {
        die("usage: %s [-march=ISA] FILE\n", argv[0]);
    }

    Buffer file_contents = read_file(filename);
    strip_comments(file_contents.p, file_contents.len);
    parse(file_contents);
    free(file_contents.p);

    return 0;
}
