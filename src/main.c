#include "common.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define MAX_TOKENS_PER_LINE 10
#define MAX_PSEUDO_EXPAND 3

// TODO: make this a run-time option
#define EXT_C 0

#define PSEUDO_LIST \
    X(PSEUDO_BEQZ,          "beqz",         FMT_REG_OFFSET      ) \
    X(PSEUDO_BGEZ,          "bgez",         FMT_REG_OFFSET      ) \
    X(PSEUDO_BGT,           "bgt",          FMT_REG_REG_OFFSET  ) \
    X(PSEUDO_BGTU,          "bgtu",         FMT_REG_REG_OFFSET  ) \
    X(PSEUDO_BGTZ,          "bgtz",         FMT_REG_OFFSET      ) \
    X(PSEUDO_BLE,           "ble",          FMT_REG_REG_OFFSET  ) \
    X(PSEUDO_BLEU,          "bleu",         FMT_REG_REG_OFFSET  ) \
    X(PSEUDO_BLEZ,          "blez",         FMT_REG_OFFSET      ) \
    X(PSEUDO_BLTZ,          "bltz",         FMT_REG_OFFSET      ) \
    X(PSEUDO_BNEZ,          "bnez",         FMT_REG_OFFSET      ) \
    X(PSEUDO_CALL,          "call",         FMT_REG_SYMBOL      ) \
    X(PSEUDO_CSRR,          "csrr",         FMT_REG_CSR         ) \
    X(PSEUDO_CSRC,          "csrc",         FMT_CSR_REG         ) \
    X(PSEUDO_CSRCI,         "csrci",        FMT_CSR_NUM         ) \
    X(PSEUDO_CSRS,          "csrs",         FMT_CSR_REG         ) \
    X(PSEUDO_CSRSI,         "csrsi",        FMT_CSR_NUM         ) \
    X(PSEUDO_CSRW,          "csrw",         FMT_CSR_REG         ) \
    X(PSEUDO_CSRWI,         "csrwi",        FMT_CSR_NUM         ) \
    X(PSEUDO_J,             "j",            FMT_OFFSET          ) \
    X(PSEUDO_JR,            "jr",           FMT_REG             ) \
    X(PSEUDO_LA,            "la",           FMT_REG_SYMBOL      ) \
    X(PSEUDO_LI,            "li",           FMT_REG_NUM         ) \
    X(PSEUDO_LLA,           "lla",          FMT_REG_SYMBOL      ) \
    X(PSEUDO_MV,            "mv",           FMT_REG_REG         ) \
    X(PSEUDO_NEG,           "neg",          FMT_REG_REG         ) \
    X(PSEUDO_NOP,           "nop",          FMT_NONE            ) \
    X(PSEUDO_NOT,           "not",          FMT_REG_REG         ) \
    X(PSEUDO_RDCYCLE,       "rdcycle",      FMT_NONE            ) \
    X(PSEUDO_RDINSTRET,     "rdinstret",    FMT_NONE            ) \
    X(PSEUDO_RDTIME,        "rdtime",       FMT_NONE            ) \
    X(PSEUDO_RET,           "ret",          FMT_NONE            ) \
    X(PSEUDO_SEQZ,          "seqz",         FMT_REG_REG         ) \
    X(PSEUDO_SGTZ,          "sgtz",         FMT_REG_REG         ) \
    X(PSEUDO_SLTZ,          "sltz",         FMT_REG_REG         ) \
    X(PSEUDO_SNEZ,          "snez",         FMT_REG_REG         ) \
    X(PSEUDO_SUB,           "sub",          FMT_REG_REG_REG     ) \
    X(PSEUDO_TAIL,          "tail",         FMT_SYMBOL          ) \
    X(PSEUDO_RDCYCLEH,      "rdcycleh",     FMT_NONE            ) \
    X(PSEUDO_RDINSTRETH,    "rdinstreth",   FMT_NONE            ) \
    X(PSEUDO_RDTIMEH,       "rdtimeh",      FMT_NONE            )

typedef enum {
#define X(MNEM, STR, FMT) MNEM,
    PSEUDO_LIST
#undef X
} Pseudo;

const char * pseudo_mnemonics[] = {
#define X(MNEM, STR, FMT) STR,
    PSEUDO_LIST
#undef X
    "invalid"
};

const size_t num_pseudo_mnemonics = NELEM(pseudo_mnemonics);

#define INST_LIST_RV32I \
    X(MNEM_LUI,     "lui",      FMT_REG_NUM,                0x00000037) \
    X(MNEM_AUIPC,   "auipc",    FMT_REG_NUM,                0x00000017) \
    X(MNEM_JAL,     "jal",      FMT_REG_OFFSET_OR_OFFSET,   0x0000006f) \
    X(MNEM_JALR,    "jalr",     FMT_REG_NUM_REG,            0x00000067) \
    X(MNEM_BEQ,     "beq",      FMT_REG_REG_OFFSET,         0x00000063) \
    X(MNEM_BNE,     "bne",      FMT_REG_REG_OFFSET,         0x00001063) \
    X(MNEM_BLT,     "blt",      FMT_REG_REG_OFFSET,         0x00004063) \
    X(MNEM_BGE,     "bge",      FMT_REG_REG_OFFSET,         0x00005063) \
    X(MNEM_BLTU,    "bltu",     FMT_REG_REG_OFFSET,         0x00006063) \
    X(MNEM_BGEU,    "bgeu",     FMT_REG_REG_OFFSET,         0x00007063) \
    X(MNEM_LB,      "lb",       FMT_REG_NUM_REG,            0x00000003) \
    X(MNEM_LH,      "lh",       FMT_REG_NUM_REG,            0x00001003) \
    X(MNEM_LW,      "lw",       FMT_REG_NUM_REG,            0x00002003) \
    X(MNEM_LBU,     "lbu",      FMT_REG_NUM_REG,            0x00004003) \
    X(MNEM_LHU,     "lhu",      FMT_REG_NUM_REG,            0x00005003) \
    X(MNEM_SB,      "sb",       FMT_REG_NUM_REG,            0x00000023) \
    X(MNEM_SH,      "sh",       FMT_REG_NUM_REG,            0x00001023) \
    X(MNEM_SW,      "sw",       FMT_REG_NUM_REG,            0x00002023) \
    X(MNEM_ADDI,    "addi",     FMT_REG_REG_NUM,            0x00000013) \
    X(MNEM_SLTI,    "slti",     FMT_REG_REG_NUM,            0x00002013) \
    X(MNEM_SLTIU,   "sltiu",    FMT_REG_REG_NUM,            0x00003013) \
    X(MNEM_XORI,    "xori",     FMT_REG_REG_NUM,            0x00004013) \
    X(MNEM_ORI,     "ori",      FMT_REG_REG_NUM,            0x00006013) \
    X(MNEM_ANDI,    "andi",     FMT_REG_REG_NUM,            0x00007013) \
    X(MNEM_SLLI,    "slli",     FMT_REG_REG_NUM,            0x00001013) \
    X(MNEM_SRLI,    "srli",     FMT_REG_REG_NUM,            0x00005013) \
    X(MNEM_SRAI,    "srai",     FMT_REG_REG_NUM,            0x40005013) \
    X(MNEM_ADD,     "add",      FMT_REG_REG_REG,            0x00000033) \
    X(MNEM_SUB,     "sub",      FMT_REG_REG_REG,            0x40000033) \
    X(MNEM_SLL,     "sll",      FMT_REG_REG_REG,            0x00001033) \
    X(MNEM_SLT,     "slt",      FMT_REG_REG_REG,            0x00002033) \
    X(MNEM_SLTU,    "sltu",     FMT_REG_REG_REG,            0x00003033) \
    X(MNEM_XOR,     "xor",      FMT_REG_REG_REG,            0x00004033) \
    X(MNEM_SRL,     "srl",      FMT_REG_REG_REG,            0x00005033) \
    X(MNEM_SRA,     "sra",      FMT_REG_REG_REG,            0x40005033) \
    X(MNEM_OR,      "or",       FMT_REG_REG_REG,            0x00006033) \
    X(MNEM_AND,     "and",      FMT_REG_REG_REG,            0x00007033) \
    X(MNEM_FENCE,   "fence",    FMT_NONE_OR_IORW,           0x0000000f) \
    X(MNEM_FENCE_I, "fence.i",  FMT_NONE,                   0x0000100f) \
    X(MNEM_ECALL,   "ecall",    FMT_NONE,                   0x00000073) \
    X(MNEM_EBREAK,  "ebreak",   FMT_NONE,                   0x00100073) \
    X(MNEM_CSRRW,   "csrrw",    FMT_REG_CSR_REG,            0x00001073) \
    X(MNEM_CSRRS,   "csrrs",    FMT_REG_CSR_REG,            0x00002073) \
    X(MNEM_CSRRC,   "csrrc",    FMT_REG_CSR_REG,            0x00003073) \
    X(MNEM_CSRRWI,  "csrrwi",   FMT_REG_CSR_NUM,            0x00005073) \
    X(MNEM_CSRRSI,  "csrrsi",   FMT_REG_CSR_NUM,            0x00006073) \
    X(MNEM_CSRRCI,  "csrrci",   FMT_REG_CSR_NUM,            0x00007073)

#if EXT_C
#define INST_LIST_RV32C \
    X(MNEM_C_NOP,      "c.nop",                FMT_NONE,        0x0001) \
    X(MNEM_C_ADDI,     "c.addi",               FMT_REG_NUM,     0x0001) \
    X(MNEM_C_JAL,      "c.jal",                FMT_OFFSET,      0x2001) \
    X(MNEM_C_LI,       "c.li",                 FMT_REG_NUM,     0x4001) \
    X(MNEM_C_ADDI16SP, "c.addi16sp",           FMT_NUM,         0x6101) \
    X(MNEM_C_LUI,      "c.lui",                FMT_REG_NUM,     0x6001) \
    X(MNEM_C_SRLI,     "c.srli",               FMT_REG_NUM,     0x8001) \
    X(MNEM_C_SRAI,     "c.srai",               FMT_REG_NUM,     0x8401) \
    X(MNEM_C_ANDI,     "c.andi",               FMT_REG_NUM,     0x8801) \
    X(MNEM_C_SUB,      "c.sub",                FMT_REG_NUM,     0x8c01) \
    X(MNEM_C_XOR,      "c.xor",                FMT_REG_NUM,     0x8c21) \
    X(MNEM_C_OR,       "c.or",                 FMT_REG_REG,     0x8c41) \
    X(MNEM_C_AND,      "c.and",                FMT_REG_REG,     0x8c61) \
    X(MNEM_C_J,        "c.j",                  FMT_OFFSET,      0xa001) \
    X(MNEM_C_BEQZ,     "c.beqz",               FMT_REG_NUM,     0xc001) \
    X(MNEM_C_BNEZ,     "c.bnez",               FMT_REG_NUM,     0xe001) \
    X(MNEM_C_ADDI4SPN, "c.addi4spn",           FMT_REG_NUM,     0x0000) \
    X(MNEM_C_FLD,      "c.fld",                FMT_REG_REG_NUM, 0x2000) \
    X(MNEM_C_LW,       "c.lw",                 FMT_REG_REG_NUM, 0x4000) \
    X(MNEM_C_FLW,      "c.flw",                FMT_REG_REG_NUM, 0x6000) \
    X(MNEM_C_FSD,      "c.fsd",                FMT_REG_REG_NUM, 0xa000) \
    X(MNEM_C_SW,       "c.sw",                 FMT_REG_REG_NUM, 0xc000) \
    X(MNEM_C_FSW,      "c.fsw",                FMT_REG_REG_NUM, 0xe000) \
    X(MNEM_C_SLLI,     "c.slli",               FMT_REG_NUM,     0x0002) \
    X(MNEM_C_FLDSP,    "c.fldsp",              FMT_REG_NUM,     0x2002) \
    X(MNEM_C_LWSP,     "c.lwsp",               FMT_REG_NUM,     0x3002) \
    X(MNEM_C_FLWSP,    "c.flwsp",              FMT_REG_NUM,     0x6002) \
    X(MNEM_C_JR,       "c.jr",                 FMT_REG,         0x8002) \
    X(MNEM_C_MV,       "c.mv",                 FMT_REG_REG,     0x8002) \
    X(MNEM_C_EBREAK,   "c.ebreak",             FMT_NONE,        0x9002) \
    X(MNEM_C_JALR,     "c.jalr",               FMT_REG,         0x9002) \
    X(MNEM_C_ADD,      "c.add",                FMT_REG_REG,     0x9002) \
    X(MNEM_C_FSDSP,    "c.fsdsp",              FMT_REG_NUM,     0xa002) \
    X(MNEM_C_SWSP,     "c.swsp",               FMT_REG_NUM,     0xc002) \
    X(MNEM_C_FSWSP,    "c.fswsp",              FMT_REG_NUM,     0xe002)
#else
#define INST_LIST_RV32C
#endif

#define INST_LIST \
    INST_LIST_RV32I \
    INST_LIST_RV32C

typedef enum {
#define X(MNEM, STR, FMT, OPCODE) MNEM,
    INST_LIST
#undef X
} Mnemonic;

const char * mnemonics[] = {
#define X(MNEM, STR, FMT, OPCODE) STR,
    INST_LIST
#undef X
    "invalid"
};

const size_t num_mnemonics = NELEM(mnemonics);

uint32_t
opcodes[] = {
#define X(MNEM, STR, FMT, OPCODE) OPCODE,
    INST_LIST
#undef X
};

// OFFSET: NUMBER or IDENT
typedef enum {
    FMT_NONE,
    FMT_NONE_OR_IORW,           // fence [pred,succ]
    FMT_REG_OFFSET_OR_OFFSET,   // jal [rd,]offset
    FMT_REG_NUM,                // auipc rd,imm ; lui rd,imm ; li rd,imm
    FMT_REG_REG_REG,
    FMT_REG_REG_OFFSET,
    FMT_REG_REG_NUM,            // slli rd,rs1,shamt
    FMT_REG_NUM_REG,
    FMT_REG_CSR_REG,
    FMT_REG_CSR_NUM,
    FMT_REG,
    FMT_REG_REG,
    FMT_OFFSET,
    FMT_NUM,
    FMT_INVALID
} Format;
#define FMT_TODO FMT_INVALID

// TODO: maybe just make this an array
static Format
format_for_mnemonic(Mnemonic mnemonic)
{
    switch (mnemonic) {
#define X(MNEM, STR, FMT, OPCODE) case MNEM: return FMT;
        INST_LIST
#undef X
        default: assert(0);
    }
    assert(0);
    return FMT_INVALID;
}

/* TODO: decide if this should detect invalid register names */
static int
reg_name_to_bits(const char * reg_name)
{
    assert(reg_name[0] != '\0' && reg_name[1] != '\0');

    if (reg_name[0] == 'x') {
        int n = atoi(reg_name+1);
        assert(n >= 0 && n <= 31);
        return n;
    } else if (strcmp(reg_name, "ra") == 0) {
        return 1;
    } else if (strcmp(reg_name, "sp") == 0) {
        return 2;
    } else if (strcmp(reg_name, "gp") == 0) {
        return 3;
    } else if (strcmp(reg_name, "tp") == 0) {
        return 4;
    } else if (strcmp(reg_name, "fp") == 0) {
        return 8;
    } else if (reg_name[0] == 'a') {
        int n = atoi(reg_name+1);
        assert(n >= 0 && n <= 7);
        return n+10;
    } else if (reg_name[0] == 's') {
        int n = atoi(reg_name+1);
        assert(n >= 0 && n <= 11);
        if (n <= 1)
            return n+8;
        else
            return n-2+18;
    } else if (reg_name[0] == 't') {
        int n = atoi(reg_name+1);
        assert(n >= 0 && n <= 6);
        if (n <= 2)
            return n+5;
        else
            return n-3+28;
    } else {
        assert(0);
    }

    return 0;
}

/* TODO: create a macro for the common operation here */
static uint32_t
b_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= ((imm >> 12) & 1) << 31;
    imm_fmt |= ((imm >> 5) & 0x3f) << 25;
    imm_fmt |= ((imm >> 1) & 0xf) << 8;
    imm_fmt |= ((imm >> 11) & 0x1) << 7;
    return imm_fmt;
}

static uint32_t
i_fmt_imm(uint32_t imm)
{
    return (imm & 0xfff) << 20;
}

static uint32_t
u_fmt_imm(uint32_t imm)
{
    return (imm & 0xfffff) << 12;
}

static uint32_t
s_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= ((imm >> 5) & 0x7f) << 25;
    imm_fmt |= (imm & 0x1f) << 7;
    return imm_fmt;
}

static uint32_t
j_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= ((imm >> 20) & 1) << 19;
    imm_fmt |= ((imm >> 1) & 0x3ff) << 9;
    imm_fmt |= ((imm >> 11) & 1) << 8;
    imm_fmt |= ((imm >> 12) & 0xff);
    return imm_fmt << 12;
}

static uint32_t
ci_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= ((imm >> 5) & 1) << 12;
    imm_fmt |= ((imm >> 0) & 0x1f) << 2;
    return imm_fmt;
}

static uint32_t
cj_fmt_imm(uint32_t imm)
{
    uint32_t imm_fmt = 0;
    imm_fmt |= ((imm >> 11) & 1) << 12;
    imm_fmt |= ((imm >> 4) & 1) << 11;
    imm_fmt |= ((imm >> 8) & 3) << 9;
    imm_fmt |= ((imm >> 10) & 1) << 8;
    imm_fmt |= ((imm >> 6) & 1) << 7;
    imm_fmt |= ((imm >> 7) & 1) << 6;
    imm_fmt |= ((imm >> 1) & 7) << 3;
    imm_fmt |= ((imm >> 5) & 1) << 2;
    //printf("cj: %d -> %d\n", imm, imm_fmt);
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
resolve_refs(uint32_t *  output)
{
    for (size_t i = 0; i < num_refs; i++) {
        uint32_t word_idx = refs[i].addr/4;
        uint32_t opcode = output[word_idx];

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
        output[word_idx] = opcode;
    }
}

static void
expect_n_tokens(int num_tokens, int n, int ln)
{
    if (num_tokens != n)
        die("error: unexpected tokens on line %d\n", ln);
}

static void
deposit_byte(uint32_t * output, size_t addr, uint32_t data)
{
    uint32_t word = output[addr/4];
    int i = addr % 4;
    word &= ~(0xff << i*8);
    word |= (data & 0xff) << i*8;
    output[addr/4] = word;
}

static void
deposit(uint32_t * output, size_t addr, size_t size, uint64_t data)
{
    assert(size <= 8);
    for (size_t i = 0; i < size; i++, addr++) {
        deposit_byte(output, addr, data);
        data >>= 8;
    }
}

static size_t
parse_instr(Token * tokens, size_t num_tokens, uint32_t * output, size_t curr_addr, int ln)
{
    Mnemonic mnemonic = str_idx_in_list(tokens[0].s, mnemonics, num_mnemonics);
    if (mnemonic == num_mnemonics)
        die("error: invalid mnemonic '%s' on line %d\n", tokens[0].s, ln);
    int compressed = strncmp(tokens[0].s, "c.", 2) == 0;
    uint32_t opcode = opcodes[mnemonic];
    Format fmt = format_for_mnemonic(mnemonic);
    uint32_t rd, rs1, rs2, imm, imm_fmt;

    switch (fmt) {
        case FMT_NONE:
            expect_n_tokens(num_tokens, 1, ln);
            break;

        case FMT_NONE_OR_IORW:
            /* TODO */
            break;

        case FMT_REG_OFFSET_OR_OFFSET:
            if (num_tokens == 4) {
                rd = reg_name_to_bits(tokens[1].s);
                if (tokens[3].t == TOK_NUM)
                    imm = parse_int(tokens[3].s);
            } else if (num_tokens == 2) {
                rd = reg_name_to_bits("x1");
                if (tokens[1].t == TOK_NUM)
                    imm = parse_int(tokens[1].s);
            } else {
                expect_n_tokens(num_tokens, 4, ln);
            }

            if ((num_tokens == 2 && tokens[1].t == TOK_IDENT) ||
                (num_tokens == 4 && tokens[3].t == TOK_IDENT)) {
                // TODO: handle num_tokens == 2 case
                add_ref(tokens[3].s, REF_J, curr_addr, ln);
                imm = 0; // TODO
            }

            imm_fmt = j_fmt_imm(imm);
            opcode |= imm_fmt | (rd << 7);
            break;

        case FMT_REG_NUM:
            // TODO: confirm rd is correct for compressed
            expect_n_tokens(num_tokens, 4, ln);
            rd  = reg_name_to_bits(tokens[1].s);
            imm = parse_int(tokens[3].s);
            imm_fmt = compressed ? ci_fmt_imm(imm) : u_fmt_imm(imm);
            opcode |= imm_fmt | (rd << 7);
            break;

        case FMT_REG_REG_REG:
            expect_n_tokens(num_tokens, 6, ln);
            rd  = reg_name_to_bits(tokens[1].s);
            rs1 = reg_name_to_bits(tokens[3].s);
            rs2 = reg_name_to_bits(tokens[5].s);
            opcode |= (rs2 << 20) | (rs1 << 15) | (rd << 7);
            break;

        case FMT_REG_REG_OFFSET:
            expect_n_tokens(num_tokens, 6, ln);
            rs1 = reg_name_to_bits(tokens[1].s);
            rs2 = reg_name_to_bits(tokens[3].s);
            if (tokens[5].t == TOK_NUM)
                imm = parse_int(tokens[5].s);
            else {
                add_ref(tokens[5].s, REF_B, curr_addr, ln);
                imm = 0;
            }
            imm_fmt = b_fmt_imm(imm);
            opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
            break;

        case FMT_REG_REG_NUM:
            // TODO: compressed
            expect_n_tokens(num_tokens, 6, ln);
            rd = reg_name_to_bits(tokens[1].s);
            rs1 = reg_name_to_bits(tokens[3].s);
            imm = parse_int(tokens[5].s);
            opcode |= (imm << 20) | (rs1 << 15) | (rd << 7);
            break;

        case FMT_REG_NUM_REG:
            expect_n_tokens(num_tokens, 7, ln);
            imm = parse_int(tokens[3].s);
            rs1 = reg_name_to_bits(tokens[5].s);
            if (mnemonic == MNEM_SW) {
                rs2 = reg_name_to_bits(tokens[1].s);
                imm_fmt = s_fmt_imm(imm);
                opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
            } else {
                rd = reg_name_to_bits(tokens[1].s);
                imm_fmt = i_fmt_imm(imm);
                opcode |= imm_fmt | (rs1 << 15) | (rd << 7);
            }
            break;

        case FMT_REG_CSR_REG:
            /* TODO */
            break;

        case FMT_REG_CSR_NUM:
            /* TODO */
            break;

        case FMT_OFFSET:
            // TODO
            if (tokens[1].t == TOK_NUM)
                imm = parse_int(tokens[1].s);
            else {
                add_ref(tokens[1].s, REF_CJ, curr_addr, ln);
                imm = 0;
            }
            if (compressed) {
                opcode |= cj_fmt_imm(imm);
            } else {
                assert(0);
            }
            break;

        case FMT_REG_REG:
            if (compressed) {
                // TODO: make sure rd and rs2 are correct
                rd = reg_name_to_bits(tokens[1].s);
                rs2 = reg_name_to_bits(tokens[3].s);
                opcode |= (rd << 7) | (rs2 << 2);
            } else {
                assert(0);
            }
            break;

        case FMT_REG:
            if (compressed) {
                rs1 = reg_name_to_bits(tokens[1].s);
                opcode |= (rs1 << 7);
            } else {
                assert(0);
            }
            break;

        default:
            assert(0);
            break;
    }

    //assert(curr_addr/4 < NELEM(output));
    size_t opcode_size = compressed ? 2: 4;
    deposit(output, curr_addr, opcode_size, opcode);
    //printf("deposit %08x (%zu) %s\n", opcode, opcode_size, mnemonics[mnemonic]);
    curr_addr += opcode_size;
    return curr_addr;
}

static size_t
parse_directive(Token * tokens, size_t num_tokens, uint32_t * output, size_t curr_addr, int ln)
{
    assert(tokens[0].t == TOK_DIR);
    if (strcmp(tokens[0].s, ".byte") == 0) {
        deposit_byte(output, curr_addr++, parse_int(tokens[1].s));
    } else if (strcmp(tokens[0].s, ".half") == 0) {
        int n = parse_int(tokens[1].s);
        deposit(output, curr_addr, 2, n);
        curr_addr += 2;
    } else if (strcmp(tokens[0].s, ".word") == 0) {
        deposit(output, curr_addr, 4, parse_int(tokens[1].s));
        curr_addr += 4;
    } else if (strcmp(tokens[0].s, ".dword") == 0) {
        int64_t n = strtoll(tokens[1].s, NULL, 0);
        deposit(output, curr_addr, 8, n);
        curr_addr += 8;
    } else if (strcmp(tokens[0].s, ".string") == 0) {
        for (const char * cp = tokens[1].s+1; *cp != '"'; cp++) {
            deposit_byte(output, curr_addr++, *cp);
        }
        deposit_byte(output, curr_addr++, '\0');
    } else if (strcmp(tokens[0].s, ".align") == 0) {
        // TODO
    } else if (strcmp(tokens[0].s, ".globl") == 0) {
        // TODO
    } else if (strcmp(tokens[0].s, ".text") == 0) {
        // TODO
    } else if (strcmp(tokens[0].s, ".data") == 0) {
        // TODO
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
compress_if_possible(Token * tokens, size_t num_tokens)
{
    Mnemonic mnemonic = str_idx_in_list(tokens[0].s, mnemonics, num_mnemonics);
    assert(mnemonic < num_mnemonics);
    if (mnemonic == MNEM_ADD) {
        // add      rd, rs1, rs2
        // c.add    rd, rs2         when rd=rs1 (invalid if rd=x0 or rs2=x0)
        // c.mv     rd, rs2         when rd=x0  (invalid if rs2=x0)
        int rd = reg_name_to_bits(tokens[1].s);
        int rs1 = reg_name_to_bits(tokens[3].s);
        int rs2 = reg_name_to_bits(tokens[5].s);
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
        // addi        rd, rs1, imm
        // c.li        rd, imm      when rs1=x0
        // c.addi      rd, imm      when rd=rs1
        // c.addi16sp  imm          when rd=rs1=x2 (invalid if imm=0)
        // c.addi4spn  rd-8, uimm   when rs1=x2    (invalid if uimm=0)
        // NOTE: addi can also be compressed to c.mv when imm=0
        int rd = reg_name_to_bits(tokens[1].s);
        int rs1 = reg_name_to_bits(tokens[3].s);
        int imm = parse_int(tokens[5].s);
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
            tokens[3] = tokens[5];
            num_tokens -= 2;
        } else if (imm == 0) {
            strcpy(tokens[0].s, "c.mv");
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_AND) {
        // and      rd, rs1, rs2
        // c.and    rd, rs2         when rd=rs1
        int rd = reg_name_to_bits(tokens[1].s);
        int rs1 = reg_name_to_bits(tokens[3].s);
        if ((rd == rs1) && (rd >= 8)) {
            strcpy(tokens[0].s, "c.and");
            sprintf(tokens[1].s, "x%d", rd-8);
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_ANDI) {
        // andi     rd, rs1, imm
        // c.andi   rd, imm         when rd=rs1
        int rd = reg_name_to_bits(tokens[1].s);
        int rs1 = reg_name_to_bits(tokens[3].s);
        if ((rd == rs1) && (rd >= 8)) {
            strcpy(tokens[0].s, "c.andi");
            sprintf(tokens[1].s, "x%d", rd-8);
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_BEQ) {
        // beq      rs1, rs2, offset
        // c.beqz   rs1, offset     when rs2=x0
        int rs2 = reg_name_to_bits(tokens[3].s);
        if (rs2 == 0) {
            strcpy(tokens[0].s, "c.beqz");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_EBREAK) {
        // c.ebreak
        strcpy(tokens[0].s, "c.ebreak");
    } else if (mnemonic == MNEM_JAL) {
        // jal      rd, offset          ; if rd is omitted, x1
        // c.j      offset          when rd=x0
        // c.jal    offset          when rd=x1
        int rd = reg_name_to_bits(tokens[1].s);
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
        // jalr     rd, offset(rs1)     ; if rd is omitted, x1
        // c.jr     rs1             when rd=x0 and offset=0
        // c.jalr   rs1             when rd=x1 and offset=0
        int offset = parse_int(tokens[3].s);
        if (offset == 0) {
            int rd = reg_name_to_bits(tokens[1].s);
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
        // c.lui    rd, imm
        // TODO
    } else if (mnemonic == MNEM_SW) {
        // sw       rs2, offset(rs1)
        // c.swsp   rs2, offset
        // c.sw     rs2, offset(rs1)
        // TODO
    } else if (mnemonic == MNEM_SLLI) {
        // slli     rd, rs1, shamt
        // c.slli   rd, shamt
        // TODO
    } else if (mnemonic == MNEM_SRAI) {
        // srai     rd, rs1, shamt
        // c.srai   rd, shamt
        // TODO
    } else if (mnemonic == MNEM_SRLI) {
        // srli     rd, rs1, shamt
        // c.srli   rd, shamt
        // TODO
    } else if (mnemonic == MNEM_XOR) {
        // xor      rd, rs1, rs2
        // c.xor    rd, rs2
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
parse(Buffer buffer)
{
    // TODO: should be a dict
    num_symbols = 0;
    symbols_cap = 10;
    symbols = malloc(sizeof(*symbols)*symbols_cap);

    num_refs = 0;
    refs_cap = 10;
    refs = malloc(sizeof(*refs)*refs_cap);

    uint32_t output[1024]; // TODO: make this dynamic
    uint32_t curr_addr = 0;
    Token tokens[MAX_TOKENS_PER_LINE];
    Token expanded_tokens[MAX_PSEUDO_EXPAND][MAX_TOKENS_PER_LINE];
    size_t num_expanded_tokens[MAX_PSEUDO_EXPAND];
    TokenizerState ts = init_tokenizer(buffer);
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
                // TODO: only if .option rvc
#if EXT_C
                num_expanded_tokens[r] = compress_if_possible(expanded_tokens[r], num_expanded_tokens[r]);
#endif
                curr_addr = parse_instr(expanded_tokens[r], num_expanded_tokens[r], output, curr_addr, ts.ln);
            } else if (expanded_tokens[r][0].t == TOK_DIR) {
                curr_addr = parse_directive(expanded_tokens[r], num_expanded_tokens[r], output, curr_addr, ts.ln);
            } else {
                assert(0);
            }
        }
    }

    resolve_refs(output);

    for (size_t i = 0; i < (curr_addr+3)/4; i++) {
        printf("%08x\n", output[i]);
        //printf("%02lx: 0x%08x\n", i*4, output[i]);
    }

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
    int comment_flag = 0;
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

int
main(int argc, char * argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
        exit(1);
    }

    Buffer file_contents = read_file(argv[1]);
    strip_comments(file_contents.p, file_contents.n);
    parse(file_contents);
    free(file_contents.p);

    return 0;
}
