#define _POSIX_C_SOURCE 199309L
#include "timer.h"
#include "risc_v.h"
#include "common.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_TOKENS_PER_LINE 10
#define MAX_PSEUDO_EXPAND 3

#define EXT_M 1
#define EXT_A 2
#define EXT_F 4
#define EXT_D 8
#define EXT_C 16
int extensions = 0;

enum {
    ISA_RV32,
    ISA_RV64
};
int isa = ISA_RV32;

#define OPTION_RVC 1
#define OPTION_PIC 2
#define OPTION_RELAX 4
int option_stack[32] = { OPTION_RVC };
size_t option_sp = 0;

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

#if 1
static int
reg_name_to_bits(String reg_name, int ln)
{
    if (reg_name.len < 2)
        goto error;

    if (reg_name.len == 2) {
        if (!strncmp(reg_name.data, "ra", 2)) return 1;
        if (!strncmp(reg_name.data, "sp", 2)) return 2;
        if (!strncmp(reg_name.data, "gp", 2)) return 3;
        if (!strncmp(reg_name.data, "tp", 2)) return 4;
        if (!strncmp(reg_name.data, "fp", 2)) return 8;
    }

    int n;
    String reg_num_str = { .data = reg_name.data+1, .len = reg_name.len-1 };
    if (parse_int(reg_num_str, &n) < 0)
        goto error;

    switch (reg_name.data[0]) {
        case 'x':
            if (n < 0 || n > 31)
                goto error;
            return n;
        case 'a':
            if (n < 0 || n > 7)
                goto error;
            return n+10;
        case 's':
            if (n < 0 || n > 11)
                goto error;
            return (n <= 1) ? n+8 : n-2+18;
        case 't':
            if (n < 0 || n > 6)
                goto error;
            return (n <= 2) ? n+5 : n-3+28;
    }

error:
    die("error: invalid register name '%.*s' on line %d\n", LEN_DATA(reg_name), ln);
    return 0;
}
#else
static int
reg_name_to_bits(String reg_name, int ln)
{
    assert(num_reg_names == 64);
    size_t i;
    for (i = 0; i < num_reg_names; i++) {
        if (string_equal(reg_name, reg_names[i]))
            break;
    }
    if (i < 32)
        return i;
    else if (i == 32)
        return 8;
    else if (i < 64)
        return i-32;
    die("error: invalid register name '%.*s' on line %d\n", LEN_DATA(reg_name), ln);
    return 0;
}
#endif

static Mnemonic
which_mnemonic(String str)
{
    for (size_t i = 0; i < num_mnemonics; i++) {
        if (string_equal(str, mnemonics[i]))
            return i;
    }
    return 0;
}

static Pseudo
which_pseudo(String str)
{
    for (size_t i = 0; i < num_pseudo_mnemonics; i++) {
        if (string_equal(str, pseudo_mnemonics[i]))
            return i;
    }
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
    String str;
    uint32_t addr;
    int ln;
} Symbol;

typedef enum {
    REF_J,
    REF_B,
    REF_CJ
} RefType;

typedef struct {
    String label;
    RefType type;
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
lookup_symbol(String str)
{
    size_t i;
    for (i = 0; i < num_symbols; i++) {
        if (string_equal(symbols[i].str, str))
            break;
    }
    return i;
}

static void
add_symbol(String str, uint32_t addr, int ln)
{
    size_t idx = lookup_symbol(str);

    if (idx < num_symbols)
        die("error: symbol '%.*s' defined on line %d already defined on line %d\n", LEN_DATA(str), ln, symbols[idx].ln);

    symbols[num_symbols++] = (Symbol) {
        .str = str,
        .addr = addr,
        .ln = ln
    };

    if (num_symbols >= symbols_cap) {
        symbols_cap *= 2;
        symbols = realloc(symbols, sizeof(*symbols)*symbols_cap);
    }
}

static void
add_ref(String label, RefType type, uint32_t addr, int ln)
{
    refs[num_refs++] = (Ref) {
        .label  = label,
        .type   = type,
        .addr   = addr,
        .ln     = ln
    };

    if (num_refs >= refs_cap) {
        refs_cap *= 2;
        refs = realloc(refs, sizeof(*refs)*refs_cap);
    }
}

#if 0
static void
print_refs_and_symbols()
{
    printf("\nsymbol table:\n");
    for (size_t i = 0; i < num_symbols; i++) {
        printf("\t%.*s: %08x\n", LEN_DATA(symbols[i].str), symbols[i].addr);
    }
    printf("\nreferences:\n");
    for (size_t i = 0; i < num_refs; i++) {
        printf("\t%.*s (%d): %02x\n", LEN_DATA(refs[i].label), refs[i].type, refs[i].addr);
    }
}
#endif

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
        size_t j = lookup_symbol(refs[i].label);
        if (j == num_symbols)
            die("error: undefined symbol %.*s on line %d\n", LEN_DATA(refs[i].label), refs[i].ln);
        uint32_t offset = symbols[j].addr - refs[i].addr;

        switch (refs[i].type) {
            case REF_J: opcode |= j_fmt_imm(offset); break;
            case REF_B: opcode |= b_fmt_imm(offset); break;
            case REF_CJ: {
                uint32_t imm_fmt = cj_fmt_imm(offset);
                if (refs[i].addr % 4 == 2)
                    imm_fmt <<= 16;
                opcode |= imm_fmt;
                break;
            }
            default: assert(0);
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
        if (fmt[i] == ' ')
            continue;
        // too few tokens
        if (ti >= num_tokens)
            return false;
        TokenType tt = tokens[ti++].type;
        switch (fmt[i]) {
            case ',':  if (tt != ',')           return false; break;
            case ':':  if (tt != ':')           return false; break;
            case '(':  if (tt != '(')           return false; break;
            case ')':  if (tt != ')')           return false; break;
            case '\n': if (tt != '\n')          return false; break;
            case 'd':  if (tt != TOK_DIR)       return false; break;
            case 'm':  if (tt != TOK_MNEM)      return false; break;
            case 'r':  if (tt != TOK_REG)       return false; break;
            case 'c':  if (tt != TOK_CSR)       return false; break;
            case 'n':  if (tt != TOK_NUM)       return false; break;
            case 'i':  if (tt != TOK_IDENT)     return false; break;
            case 's':  if (tt != TOK_STRING)    return false; break;
            case '%':  if (tt != TOK_REL)       return false; break;
            case 'o':  if (tt != TOK_NUM && tt != TOK_IDENT)    return false; break;
            default: assert(0);
        }
    }
    // extra tokens left over
    if (ti < num_tokens)
        return false;
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
        if (tokens[i].type != va_arg(ap, TokenType))
            return false;
    }
    va_end(ap);
    return true;
}
#endif

static int
parse_int_or_die(String str, int ln)
{
    int i;
    int errnum = parse_int(str, &i);
    if (errnum < 0)
        die("error: %s on line %d\n", parse_int_strerror(errnum), ln);
    return i;
}

static int
parse_iorw_or_die(String str, int ln)
{
    int iorw = 0;
    for (size_t i = 0; i < str.len; i++) {
        char c = str.data[i];
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
    Mnemonic mnemonic = which_mnemonic(tokens[0].str);
    if (mnemonic == num_mnemonics)
        die("error: invalid mnemonic '%.*s' on line %d\n", LEN_DATA(tokens[0].str), ln);
    bool compressed = (tokens[0].str.len >= 2) && !strncmp(tokens[0].str.data, "c.", 2);
    uint32_t opcode = opcodes[mnemonic];
    Operands expected_operands = operands_for_mnemonic[mnemonic];
    int32_t imm;
    uint32_t rd, rs1, rs2;
    uint32_t imm_fmt, pred, succ;
    uint32_t csr;

    if (compressed && !compressed_available())
        die("error: compressed instruction used but compressed extension not enabled on line %d\n", ln);

    switch (expected_operands) {
        case OPERANDS_NONE:
            tokens_match_or_die(tokens, num_tokens, "m", ln);
            break;

        case OPERANDS_IORW_IORW:
            if (num_tokens == 4) {
                tokens_match_or_die(tokens, num_tokens, "m i,i", ln);
                pred = parse_iorw_or_die(tokens[1].str, ln);
                succ = parse_iorw_or_die(tokens[3].str, ln);
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
                rd = reg_name_to_bits(tokens[1].str, ln);
                if (tokens[3].type == TOK_NUM)
                    imm = parse_int_or_die(tokens[3].str, ln);
                else if (tokens[3].type == TOK_IDENT)
                    add_ref(tokens[3].str, REF_J, curr_addr, ln);
                else
                    die("error: invalid format for %.*s instruction on line %d\n", LEN_DATA(tokens[0].str), ln);
            } else {
                tokens_match_or_die(tokens, num_tokens, "m o", ln);
                rd = reg_name_to_bits(STRING("x1"), ln);
                if (tokens[1].type == TOK_NUM)
                    imm = parse_int_or_die(tokens[1].str, ln);
                else if (tokens[1].type == TOK_IDENT)
                    add_ref(tokens[1].str, REF_J, curr_addr, ln);
                else
                    die("error: invalid format for %.*s instruction on line %d\n", LEN_DATA(tokens[0].str), ln);
            }
            opcode |= j_fmt_imm(imm) | (rd << 7);
            break;

        case OPERANDS_REG_NUM:
            if (num_tokens == 7) {
                tokens_match_or_die(tokens, num_tokens, "m r,%(i)", ln);
                die("%% relocations not yet implemented\n");
            } else {
                tokens_match_or_die(tokens, num_tokens, "m r,n", ln);
            }
            rd  = reg_name_to_bits(tokens[1].str, ln);
            imm = parse_int_or_die(tokens[3].str, ln);
            if ((imm < 0) || (imm > 0xfffff))
                die("error: lui immediate must be in range 0..1048575 on line %d\n", ln);
            imm_fmt = compressed ? ci_fmt_imm(imm) : u_fmt_imm(imm);
            opcode |= imm_fmt | (rd << 7);
            break;

        case OPERANDS_REG_REG_REG:
            tokens_match_or_die(tokens, num_tokens, "m r,r,r", ln);
            rd  = reg_name_to_bits(tokens[1].str, ln);
            rs1 = reg_name_to_bits(tokens[3].str, ln);
            rs2 = reg_name_to_bits(tokens[5].str, ln);
            opcode |= (rs2 << 20) | (rs1 << 15) | (rd << 7);
            break;

        case OPERANDS_REG_REG_OFFSET:
            tokens_match_or_die(tokens, num_tokens, "m r,r,o", ln);
            rs1 = reg_name_to_bits(tokens[1].str, ln);
            rs2 = reg_name_to_bits(tokens[3].str, ln);
            if (tokens[5].type == TOK_NUM)
                imm = parse_int_or_die(tokens[5].str, ln);
            else {
                add_ref(tokens[5].str, REF_B, curr_addr, ln);
                imm = 0;
            }
            imm_fmt = b_fmt_imm(imm);
            opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
            break;

        case OPERANDS_REG_REG_NUM:
            tokens_match_or_die(tokens, num_tokens, "m r,r,n", ln);
            rd = reg_name_to_bits(tokens[1].str, ln);
            rs1 = reg_name_to_bits(tokens[3].str, ln);
            imm = parse_int_or_die(tokens[5].str, ln);
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
            if (string_equal(tokens[0].str, STRING("c.addi4spn"))) {
                if (rs1 != 2)
                    die("error: register source must be sp for %.*s on line %d\n", LEN_DATA(tokens[0].str), ln);
            }
            break;

        case OPERANDS_REG_NUM_REG:
            tokens_match_or_die(tokens, num_tokens, "m r,n(r)", ln);
            imm = parse_int_or_die(tokens[3].str, ln);
            rs1 = reg_name_to_bits(tokens[5].str, ln);
            if (format_of_instr[mnemonic] == FMT_S) {
                rs2 = reg_name_to_bits(tokens[1].str, ln);
                imm_fmt = s_fmt_imm(imm);
                opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
            } else {
                rd = reg_name_to_bits(tokens[1].str, ln);
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
            rd = reg_name_to_bits(tokens[1].str, ln);
            csr = 0; // csr_name_to_bits(tokens[3].str, ln);
            imm = parse_int_or_die(tokens[5].str, ln);
            if (imm < 0 || imm >= 1<<5) {
                die("error: csr*i immediate must be in range 0..31 on line %d\n", ln);
            }
            opcode |= (csr << 20) | (imm << 15) | (rd << 7);
            break;

        case OPERANDS_OFFSET:
            tokens_match_or_die(tokens, num_tokens, "m o", ln);
            imm = 0;
            if (tokens[1].type == TOK_NUM)
                imm = parse_int_or_die(tokens[1].str, ln);
            else
                add_ref(tokens[1].str, REF_CJ, curr_addr, ln);
            assert(compressed);
            opcode |= cj_fmt_imm(imm);
            break;

        // c.add, c.and, c.or, c.mv
        case OPERANDS_REG_REG:
            tokens_match_or_die(tokens, num_tokens, "m r,r", ln);
            assert(compressed);
            // TODO: make sure rd and rs2 are correct
            rd = reg_name_to_bits(tokens[1].str, ln);
            rs2 = reg_name_to_bits(tokens[3].str, ln);
            if (mnemonic == MNEM_C_ADD) {
                if ((rd == 0) || (rs2 == 0))
                    die("error: rd and rs2 must not be 0 for %.*s instruction on line %d\n", LEN_DATA(tokens[0].str), ln);
            } else if (mnemonic == MNEM_C_MV) {
                if (rs2 == 0)
                    die("error: rs2 must not be 0 for %.*s instruction on line %d\n", LEN_DATA(tokens[0].str), ln);
            } else if ((mnemonic == MNEM_C_AND) || (mnemonic == MNEM_C_OR)) {
                // TODO: check register ranges
            }
            opcode |= (rd << 7) | (rs2 << 2);
            break;

        case OPERANDS_REG:
            tokens_match_or_die(tokens, num_tokens, "m r", ln);
            assert(compressed);
            rs1 = reg_name_to_bits(tokens[1].str, ln);
            opcode |= (rs1 << 7);
            break;

        case OPERANDS_NUM:
            tokens_match_or_die(tokens, num_tokens, "m r,n", ln); // NOTE: match gcc
            rd = reg_name_to_bits(tokens[1].str, ln);
            if (rd != 2)
                die("error: register destination must be sp for %.*s on line %d\n", LEN_DATA(tokens[0].str), ln);
            break;

        default:
            die("error: don't know how to parse %.*s on line %d\n", LEN_DATA(tokens[0].str), ln);
            assert(0);
            break;
    }

    size_t opcode_size = compressed ? 2: 4;
    deposit(output, curr_addr, opcode_size, opcode);
    //printf("deposit %08x (%zu) %.*s\n", opcode, opcode_size, LEN_DATA(mnemonics[mnemonic]));
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
handle_data_directive(size_t nbytes, Token * tokens, size_t num_tokens, Buffer * output, size_t curr_addr, int ln)
{
    //tokens_match_or_die(tokens, num_tokens, "d n", ln);
    if (num_tokens % 2 != 0) {
        die("error: parse error on line %d\n", ln);
    }
    for (size_t i = 0; i < num_tokens; i++) {
        if (i == 0) {
        } else if (i % 2 == 1) {
            int n = parse_int_or_die(tokens[i].str, ln);
            // TODO: bounds check
            // TODO: confirm nbytes == 8 case
            deposit(output, curr_addr, nbytes, n);
            curr_addr += nbytes;
        } else if (i % 2 == 0) {
            if (tokens[i].type != ',')
                die("error: parse error on line %d\n", ln);
        }
    }
    return curr_addr;
}

static size_t
parse_directive(Token * tokens, size_t num_tokens, Buffer * output, size_t curr_addr, int ln)
{
    assert(tokens[0].type == TOK_DIR);
    // TODO: bounds checks

    size_t data_sz = 0;
    if      (string_equal(tokens[0].str, STRING(".byte")))  data_sz = 1;
    else if (string_equal(tokens[0].str, STRING(".half")))  data_sz = 2;
    else if (string_equal(tokens[0].str, STRING(".word")))  data_sz = 4;
    else if (string_equal(tokens[0].str, STRING(".dword"))) data_sz = 8;

    if (data_sz > 0) {
        curr_addr = handle_data_directive(data_sz, tokens, num_tokens, output, curr_addr, ln);
    } else if (string_equal(tokens[0].str, STRING(".string"))) {
        tokens_match_or_die(tokens, num_tokens, "d s", ln);
        for (const char * cp = tokens[1].str.data+1; *cp != '"'; cp++) {
            deposit(output, curr_addr++, 1, *cp);
        }
        deposit(output, curr_addr++, 1, '\0');
    } else if (string_equal(tokens[0].str, STRING(".align"))) {
        tokens_match_or_die(tokens, num_tokens, "d n", ln);
        int log2_alignment = parse_int_or_die(tokens[1].str, ln);
        if ((log2_alignment < 1) || (log2_alignment > 20))
            die("error: invalid alignment %d on line %d\n", log2_alignment, ln);
        curr_addr = align_output(output, 1 << log2_alignment, curr_addr);
    } else if (string_equal(tokens[0].str, STRING(".globl"))) {
        tokens_match_or_die(tokens, num_tokens, "d i", ln);
        // TODO
    } else if (string_equal(tokens[0].str, STRING(".text"))) {
        tokens_match_or_die(tokens, num_tokens, "d", ln);
        // TODO
    } else if (string_equal(tokens[0].str, STRING(".data"))) {
        tokens_match_or_die(tokens, num_tokens, "d", ln);
        // TODO
    } else if (string_equal(tokens[0].str, STRING(".option"))) {
        tokens_match_or_die(tokens, num_tokens, "d i", ln);
        if (string_equal(tokens[1].str, STRING("push"))) {
            option_sp++;
            if (option_sp >= NELEM(option_stack))
                die("error: exceeded max depth of option stack on line %d\n", ln);
            option_stack[option_sp] = option_stack[option_sp-1];
        } else if (string_equal(tokens[1].str, STRING("pop"))) {
            if (option_sp == 0)
                die("error: popped option stack too many times on line %d\n", ln);
            option_sp--;
        } else if (string_equal(tokens[1].str, STRING("rvc"))) {
            option_stack[option_sp] |= OPTION_RVC;
        } else if (string_equal(tokens[1].str, STRING("norvc"))) {
            option_stack[option_sp] &= ~OPTION_RVC;
        } else if (string_equal(tokens[1].str, STRING("pic"))) {
            option_stack[option_sp] |= OPTION_PIC;
        } else if (string_equal(tokens[1].str, STRING("nopic"))) {
            option_stack[option_sp] &= ~OPTION_PIC;
        } else if (string_equal(tokens[1].str, STRING("relax"))) {
            option_stack[option_sp] |= OPTION_RELAX;
        } else if (string_equal(tokens[1].str, STRING("norelax"))) {
            option_stack[option_sp] &= ~OPTION_RELAX;
        }
    } else {
        die("error: unsupported directive %.*s on line %d\n", LEN_DATA(tokens[0].str), ln);
    }
    return curr_addr;
}

static size_t
substitute_pseudoinstr(Token expanded_tokens[][MAX_TOKENS_PER_LINE], size_t num_expanded_tokens[], Token * tokens, size_t num_tokens, int ln)
{
    if (tokens[0].type != TOK_PSEUDO) {
        for (size_t i = 0; i < num_tokens; i++)
            expanded_tokens[0][i] = tokens[i];
        num_expanded_tokens[0] = num_tokens;
        return 1;
    }

    // TODO: tokens_match_or_die() based on OPERANDS_

    Pseudo pseudo = which_pseudo(tokens[0].str);
    switch (pseudo) {
        case PSEUDO_BEQZ:
            // beqz rs1, offset
            // - beq  rs1, x0, offset
            assert(0); // TODO
        case PSEUDO_BGEZ:
            // bgez     rs1, offset
            // - bge    rs1, x0, offset
            assert(0); // TODO
        case PSEUDO_BGT:
            // bgt      rs1, rs2, offset
            // - blt    rs2, rs1, offset
            assert(0); // TODO
        case PSEUDO_BGTU:
            // bgtu     rs1, rs2, offset
            // - bltu   rs2, rs1, offset
            assert(0); // TODO
        case PSEUDO_BGTZ:
            // bgtz     rs2, offset
            // - blt    x0, rs2, offset
            assert(0); // TODO
        case PSEUDO_BLE:
            // ble      rs1, rs2, offset
            // - bge    rs2, rs1, offset
            assert(0); // TODO
        case PSEUDO_BLEU:
            // bleu     rs1, rs2, offset
            // - bgeu   rs2, rs1, offset
            assert(0); // TODO
        case PSEUDO_BLEZ:
            // blez     rs2, offset
            // - bge    x0, rs2, offset
            assert(0); // TODO
        case PSEUDO_BLTZ:
            // bltz     rs1, offset
            // - blt    rs1, x0, offset
            assert(0); // TODO
        case PSEUDO_BNEZ:
            // bnez     rs1, offset
            // - bne    rs1, x0, offset
            assert(0); // TODO
        case PSEUDO_CALL:
            // call     rd, symbol
            // - auipc  rd, offsetHi
            // - jalr rd, offsetLo(rd) ; if rd is omitted, x1
            assert(0); // TODO
        case PSEUDO_CSRR:
            // csrr     rd, csr
            // - csrrs  rd, csr, x0
            assert(0); // TODO
        case PSEUDO_CSRC:
            // csrc     csr, rs1
            // - csrrc  x0, csr, rs1
            assert(0); // TODO
        case PSEUDO_CSRCI:
            // csrci    csr, zimm[4:0]
            // - csrrci x0, csr, zimm
            assert(0); // TODO
        case PSEUDO_CSRS:
            // csrs     csr, rs1
            // - csrrs  x0, csr, rs1
            assert(0); // TODO
        case PSEUDO_CSRSI:
            // csrsi    csr, zimm[4:0]
            // - csrrsi x0, csr, zimm
            assert(0); // TODO
        case PSEUDO_CSRW:
            // csrw     csr, rs1
            // - csrrw  x0, csr, rs1
            assert(0); // TODO
        case PSEUDO_CSRWI:
            // csrwi    csr, zimm[4:0]
            // - csrrwi x0, csr, zimm
            assert(0); // TODO
        case PSEUDO_J:
            // j        offset
            // - jal    x0, offset
            expanded_tokens[0][0] = (Token) { .type=TOK_MNEM,   .str=STRING("jal") };
            expanded_tokens[0][1] = (Token) { .type=TOK_REG,    .str=STRING("x0")  };
            expanded_tokens[0][2] = (Token) { .type=',',        .str=STRING(",")   };
            expanded_tokens[0][3] = tokens[1];
            num_expanded_tokens[0] = 4;
            return 1;

        case PSEUDO_JR:
            // jr       rs1
            // - jalr   x0, 0(rs1)
            expanded_tokens[0][0] = (Token) { .type=TOK_MNEM,   .str=STRING("jalr") };
            expanded_tokens[0][1] = (Token) { .type=TOK_REG,    .str=STRING("x0")   };
            expanded_tokens[0][2] = (Token) { .type=',',        .str=STRING(",")    };
            expanded_tokens[0][3] = (Token) { .type=TOK_NUM,    .str=STRING("0")    };
            expanded_tokens[0][4] = (Token) { .type='(',        .str=STRING("(")    };
            expanded_tokens[0][5] = tokens[1];
            expanded_tokens[0][6] = (Token) { .type=')',        .str=STRING(")")    };
            num_expanded_tokens[0] = 7;
            return 1;

        case PSEUDO_LA:
            // TODO: check ISA and PIC option
            // TODO: implement non-PIC case
            // TODO: implement RV64I
            // la       rd, symbol
            assert(isa == ISA_RV32);
            if (option_stack[option_sp] & OPTION_PIC) {
                // RV32I, PIC:
                // - auipc rd, %pcrel_hi(symbol)
                // - lw rd, rd, %pcrel_lo(symbol)
                // where
                //   %pcrel_hi: R_RISCV_PCREL_HI20:   delta[31 : 12] + delta[11]
                //   %pcrel_lo: R_RISCV_PCREL_LO12_I: delta[11:0]
                //   and delta = GOT[symbol] − pc
                expanded_tokens[0][0] = (Token) { .type=TOK_MNEM,   .str=STRING("auipc") };
                expanded_tokens[0][1] = tokens[1];
                expanded_tokens[0][2] = (Token) { .type=',',        .str=STRING(",")     };
                expanded_tokens[0][3] = (Token) { .type=TOK_NUM,    .str=STRING("0")     }; // TODO: offsetHi
                num_expanded_tokens[0] = 4;
                expanded_tokens[1][0] = (Token) { .type=TOK_MNEM,   .str=STRING("lw")    };
                expanded_tokens[1][1] = tokens[1];
                expanded_tokens[1][2] = (Token) { .type=',',        .str=STRING(",")     };
                expanded_tokens[1][3] = (Token) { .type=TOK_NUM,    .str=STRING("0")     };
                expanded_tokens[1][4] = (Token) { .type='(',        .str=STRING("(")     };
                expanded_tokens[1][5] = tokens[1];
                expanded_tokens[1][6] = (Token) { .type=')',        .str=STRING(")")     };
                num_expanded_tokens[1] = 7;
                return 2;
            } else {
                // RV32I, no-PIC:
                // - auipc rd, %pcrel_hi(symbol)
                // - addi rd, rd, %pcrel_lo(symbol)
                // where
                //   %pcrel_hi: R_RISCV_PCREL_HI20:   delta[31 : 12] + delta[11]
                //   %pcrel_lo: R_RISCV_PCREL_LO12_I: delta[11:0]
                //   and delta = symbol − pc
                expanded_tokens[0][0] = (Token) { .type=TOK_MNEM,   .str=STRING("auipc")        };
                expanded_tokens[0][1] = tokens[1];
                expanded_tokens[0][2] = (Token) { .type=',',        .str=STRING(",")            };
                expanded_tokens[0][3] = (Token) { .type=TOK_REL,    .str=STRING("%pcrel_hi")    };
                expanded_tokens[0][4] = (Token) { .type='(',        .str=STRING("(")            };
                expanded_tokens[0][5] = tokens[3];
                expanded_tokens[0][6] = (Token) { .type=')',        .str=STRING(")")            };
                num_expanded_tokens[0] = 7;
                expanded_tokens[1][0] = (Token) { .type=TOK_MNEM,   .str=STRING("addi")         };
                expanded_tokens[1][1] = tokens[1];
                expanded_tokens[1][2] = (Token) { .type=',',        .str=STRING(",")            };
                expanded_tokens[1][3] = tokens[1];
                expanded_tokens[1][4] = (Token) { .type=',',        .str=STRING(",")            };
                expanded_tokens[1][5] = (Token) { .type=TOK_REL,    .str=STRING("%pcrel_lo")    };
                expanded_tokens[1][6] = (Token) { .type='(',        .str=STRING("(")            };
                expanded_tokens[1][7] = tokens[3];
                expanded_tokens[1][8] = (Token) { .type=')',        .str=STRING(")")            };
                num_expanded_tokens[1] = 9;
                return 2;
            }
            // RV64I, PIC: TODO
            // RV64I, no-PIC: TODO
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
            expanded_tokens[0][0] = (Token) { .type=TOK_MNEM,   .str=STRING("xori") };
            expanded_tokens[0][1] = tokens[1];
            expanded_tokens[0][2] = tokens[2];
            expanded_tokens[0][3] = tokens[3];
            expanded_tokens[0][4] = (Token) { .type=',',        .str=STRING(",")    };
            expanded_tokens[0][5] = (Token) { .type=TOK_NUM,    .str=STRING("-1")   };
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
            die("error: unsupported pseudoinstruction '%.*s' on line %d\n", LEN_DATA(tokens[0].str), ln);
    }
    return 0;
}

static size_t
compress_if_possible(Token * tokens, size_t num_tokens, int ln)
{
    Mnemonic mnemonic = which_mnemonic(tokens[0].str);
    assert(mnemonic < num_mnemonics);
    if (mnemonic == MNEM_ADD) {
        // add      rd, rs1, rs2
        // - c.add  rd, rs2         when rd=rs1 (invalid if rd=x0 or rs2=x0)
        // - c.mv   rd, rs2         when rd=x0  (invalid if rs2=x0)
        int rd = reg_name_to_bits(tokens[1].str, ln);
        int rs1 = reg_name_to_bits(tokens[3].str, ln);
        int rs2 = reg_name_to_bits(tokens[5].str, ln);
        if ((rd == rs1) && (rd != 0) && (rs2 != 0)) {
            tokens[0].str = STRING("c.add");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        } else if (rs1 == 0 && rs2 != 0) {
            tokens[0].str = STRING("c.mv");
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
        int rd = reg_name_to_bits(tokens[1].str, ln);
        int rs1 = reg_name_to_bits(tokens[3].str, ln);
        int imm = parse_int_or_die(tokens[5].str, ln);
        // TODO: confirm bounds check on imm
        if ((rs1 == 0) && (imm < 64)) {
            tokens[0].str = STRING("c.li");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        } else if ((rd == rs1) && (imm < 64)) {
            tokens[0].str = STRING("c.addi");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        } else if ((rd == 2) && (rs1 == 2) && (imm != 0)) {
            tokens[0].str = STRING("c.addi16sp");
            tokens[1] = tokens[5];
            num_tokens -= 4;
        } else if ((rs1 == 2) && (imm != 0) && (rd >= 8)) {
            tokens[0].str = STRING("c.addi4spn");
            tokens[1].str = reg_names[rd-8];
            // NOTE: match gcc
            //tokens[3] = tokens[5];
            //num_tokens -= 2;
        } else if (imm == 0) {
            tokens[0].str = STRING("c.mv");
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_AND) {
        // and      rd, rs1, rs2
        // - c.and  rd, rs2         when rd=rs1
        int rd = reg_name_to_bits(tokens[1].str, ln);
        int rs1 = reg_name_to_bits(tokens[3].str, ln);
        if ((rd == rs1) && (rd >= 8)) {
            tokens[0].str = STRING("c.and");
            tokens[1].str = reg_names[rd-8];
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_ANDI) {
        // andi     rd, rs1, imm
        // - c.andi rd, imm       when rd=rs1
        int rd = reg_name_to_bits(tokens[1].str, ln);
        int rs1 = reg_name_to_bits(tokens[3].str, ln);
        if ((rd == rs1) && (rd >= 8)) {
            tokens[0].str = STRING("c.andi");
            tokens[1].str = reg_names[rd-8];
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_BEQ) {
        // beq      rs1, rs2, offset
        // - c.beqz rs1, offset     when rs2=x0
        int rs2 = reg_name_to_bits(tokens[3].str, ln);
        if (rs2 == 0) {
            tokens[0].str = STRING("c.beqz");
            tokens[3] = tokens[5];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_EBREAK) {
        // c.ebreak
        tokens[0].str = STRING("c.ebreak");
    } else if (mnemonic == MNEM_JAL) {
        // jal      rd, offset      ; if rd is omitted, x1
        // - c.j    offset          when rd=x0
        // - c.jal  offset          when rd=x1
        int rd = reg_name_to_bits(tokens[1].str, ln);
        if (rd == 0) {
            tokens[0].str = STRING("c.j");
            tokens[1] = tokens[3];
            num_tokens -= 2;
        } else if (rd == 1) {
            tokens[0].str = STRING("c.jal");
            tokens[1] = tokens[3];
            num_tokens -= 2;
        }
    } else if (mnemonic == MNEM_JALR) {
        // jalr     rd, offset(rs1) ; if rd is omitted, x1
        // - c.jr   rs1             when rd=x0 and offset=0
        // - c.jalr rs1             when rd=x1 and offset=0
        int offset = parse_int_or_die(tokens[3].str, ln);
        if (offset == 0) {
            int rd = reg_name_to_bits(tokens[1].str, ln);
            if (rd == 0) {
                tokens[0].str = STRING("c.jr");
                tokens[1] = tokens[5];
                num_tokens -= 5;
            } else if (rd == 1) {
                tokens[0].str = STRING("c.jalr");
                tokens[1] = tokens[5];
                num_tokens -= 5;
            }
        }
    } else if (mnemonic == MNEM_LUI) {
        // lui      rd, imm
        // - c.lui  rd, imm     when -32 <= sext(imm) < 32
        int imm = parse_int_or_die(tokens[3].str, ln);
        if ((imm < 0) || (imm > 0xfffff))
            die("error: lui immediate must be in range 0..1048575 on line %d\n", ln);
        int imm_sext = sext(imm, 20);
        if (imm_sext >= -32 && imm_sext < 32) {
            tokens[0].str = STRING("c.lui");
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
get_line_of_tokens(Tokenizer * tz, Token * tokens)
{
    size_t num_tokens = 0;
    while (1) {
        Token tok = get_token(tz);
        if (tok.type == '\n' || tok.type == TOK_EOF)
            break;
        if (num_tokens >= MAX_TOKENS_PER_LINE)
            die("error: too many tokens on line %d\n", tz->ln);
        tokens[num_tokens++] = tok;
    }
    return num_tokens;
}

/* TODO: separate parsing from outputting */
static Buffer
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
    Tokenizer tz = init_tokenizer(input);
    while (!tz.eof) {
        size_t num_tokens = get_line_of_tokens(&tz, tokens);
        if (num_tokens == 0)
            continue;

        // labels
        Token * tokens_no_label = tokens;
        if (num_tokens >= 2 && tokens[1].type == ':') {
            add_symbol(tokens[0].str, curr_addr, tz.ln);
            tokens_no_label = &tokens[2];
            num_tokens -= 2;
        }
        if (num_tokens == 0)
            continue;

        size_t num_pseudo_expansions = substitute_pseudoinstr(expanded_tokens, num_expanded_tokens, tokens_no_label, num_tokens, tz.ln);

        for (size_t r = 0; r < num_pseudo_expansions; r++) {
            if (expanded_tokens[r][0].type == TOK_MNEM) {
                if (compressed_available())
                    num_expanded_tokens[r] = compress_if_possible(expanded_tokens[r], num_expanded_tokens[r], tz.ln);
                curr_addr = parse_instr(expanded_tokens[r], num_expanded_tokens[r], &output, curr_addr, tz.ln);
            } else if (expanded_tokens[r][0].type == TOK_DIR) {
                curr_addr = parse_directive(expanded_tokens[r], num_expanded_tokens[r], &output, curr_addr, tz.ln);
            } else {
                die("error: cannot parse line %d\n", tz.ln);
            }
        }
    }

    resolve_refs(&output);
    //print_refs_and_symbols();

    free(symbols);
    free(refs);

    return output;
}

static void
print_output(FILE * fp, Buffer output)
{
#if 0
    for (size_t i = 0; i < output.len; i++) {
        if (i % 16 == 0) {
            if (i != 0)
                fprintf(fp, "\n");
            fprintf(fp, "%02zx: ", i);
        }
        fprintf(fp, "%02x ", (int) unpack_le(output.p + i, 1));
        //if ((i+1) % 16 == 0)
        //    fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
#endif

#if 0
    for (size_t i = 0; i < output.len/4; i++) {
        //fprintf(fp, "%02zx: %08x\n", i*4, (int) unpack_le(output.p + i*4, 4));
        fprintf(fp, "%08x\n", (int) unpack_le(output.p + i*4, 4));
    }
#else
    for (size_t i = 0; i < output.len/2; i++) {
        int b2 = (int) unpack_le(output.p + i*2, 2);
        bool compressed = (b2 & 3) != 3;
        if ((extensions & EXT_C) && compressed) {
            fprintf(fp, "%04x\n", b2);
        } else {
            int b4 = (int) unpack_le(output.p + i*2, 4);
            fprintf(fp, "%08x\n", b4);
            i++;
        }
    }
#endif
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

// TODO: enforce canonical order (imafdc)
// TODO: enforce d requires f
// TODO: parse _ (e.g. _zicsr_zifencei, etc.)
static void
parse_isa_string(const char * isa_str)
{
    if (strncmp(isa_str, "rv", 2) != 0) {
        die("error: unsupported ISA string '%s'\n", isa_str);
    }
    if ((strncmp(isa_str+2, "32", 2) != 0) && (strncmp(isa_str+2, "64", 2) != 0)) {
        die("error: unsupported ISA string '%s'\n", isa_str);
    }
    if (strncmp(isa_str+2, "64", 2) == 0) {
        die("error: RV64 is not yet supported\n", isa_str);
    }
    size_t isa_len = strlen(isa_str);
    for (size_t i = 4; i < isa_len; i++) {
        switch (isa_str[i]) {
            case 'i': break;
            case 'm': extensions |= EXT_M; break;
            case 'a': extensions |= EXT_A; break;
            case 'f': extensions |= EXT_F; break;
            case 'd': extensions |= EXT_D; break;
            case 'c': extensions |= EXT_C; break;
            default:
                die("error: unsupported extension '%c' in isa string '%s'\n", isa_str[i], isa_str);
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

#ifndef NRUNS
#define NRUNS 2
#endif

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
    Buffer output = parse(file_contents);
    print_output(stdout, output);
    free(file_contents.p);
    free(output.p);

#if PERF_TEST
    // TODO: automatically detect when min time has been reached
    Timer timer;
    long min_run_time = LONG_MAX;
    for (int run = 0; run < NRUNS; run++) {
        timer_start(&timer);
        output = parse(file_contents);
        timer_stop(&timer);
        free(output.p);
        long run_time = get_elapsed_us(&timer);
        if (run_time < min_run_time)
            min_run_time = run_time;
        //fprintf(stderr, "elapsed %ldus\n", run_time);
    }
    fprintf(stderr, "min time: %ldus\n", min_run_time);
#endif

    return 0;
}
