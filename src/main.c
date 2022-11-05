#include "common.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define MAX_TOKENS_PER_LINE 10

#define INST_LIST \
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

typedef enum {
#define X(MNEM, STR, FMT, OPCODE) MNEM,
    INST_LIST
#undef X
    MNEM_INVALID
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
    FMT_INVALID
} Format;

static Format
format_for_mnemonic(Mnemonic mnemonic)
{
    switch (mnemonic) {
#define X(MNEM, STR, FMT, OPCODE) case MNEM: return FMT;
        INST_LIST
#undef X
        case MNEM_INVALID:
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

typedef struct {
    char * s;
    uint32_t addr;
    int ln;
} Symbol;

// TODO
typedef enum {
    REF_J,
    REF_B
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
    if (idx < num_symbols) {
        fprintf(stderr, "error on line %d: symbol '%s' already defined on line %d\n", ln, s, symbols[idx].ln);
        exit(EXIT_FAILURE);
    }
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
        if (j == num_symbols) {
            fprintf(stderr, "error: undefined symbol %s on line %d\n", refs[i].s, refs[i].ln);
            exit(EXIT_FAILURE);
        }
        uint32_t offset = symbols[j].addr - refs[i].addr;

        if (refs[i].t == REF_J) {
            opcode |= j_fmt_imm(offset);
        } else if (refs[i].t == REF_B) {
            opcode |= b_fmt_imm(offset);
        } else {
            assert(0);
        }
        output[word_idx] = opcode;
    }
}

static void
expect_n_tokens(int num_tokens, int n, int ln)
{
    if (num_tokens != n) {
        fprintf(stderr, "error: unexpected tokens on line %d\n", ln);
        exit(EXIT_FAILURE);
    }
}

static void
set_byte(uint32_t * output, size_t addr, uint32_t data)
{
    uint32_t word = output[addr/4];
    int i = addr % 4;
    word &= ~(0xff << i*8);
    word |= (data & 0xff) << i*8;
    output[addr/4] = word;
}

static size_t
parse_instr(Token * tokens, size_t num_tokens, uint32_t * output, size_t curr_addr, int ln)
{
    Mnemonic mnemonic = str_idx_in_list(tokens[0].s, mnemonics, num_mnemonics);
    if (mnemonic == MNEM_INVALID) {
        fprintf(stderr, "error: invalid mnemonic on line %d\n", ln);
        exit(EXIT_FAILURE);
    }
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
                if (tokens[3].t == TOK_NUMBER)
                    imm = parse_int(tokens[3].s);
            } else if (num_tokens == 2) {
                rd = reg_name_to_bits("x1");
                if (tokens[1].t == TOK_NUMBER)
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
            expect_n_tokens(num_tokens, 4, ln);
            rd  = reg_name_to_bits(tokens[1].s);
            imm = parse_int(tokens[3].s);
            opcode |= u_fmt_imm(imm) | (rd << 7);
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
            if (tokens[5].t == TOK_NUMBER)
                imm = parse_int(tokens[5].s);
            else {
                add_ref(tokens[5].s, REF_B, curr_addr, ln);
                imm = 0;
            }
            imm_fmt = b_fmt_imm(imm);
            opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
            break;

        case FMT_REG_REG_NUM:
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

        case FMT_INVALID:
            assert(0);
            break;

        default:
            assert(0);
            break;
    }

    //assert(curr_addr/4 < NELEM(output));
    output[curr_addr/4] = opcode;
    //printf("%02x: 0x%08x\n", curr_addr, opcode);
    curr_addr += 4;
    return curr_addr;
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
    size_t num_words = 0;
    uint32_t curr_addr = 0;
    Token tokens[MAX_TOKENS_PER_LINE];
    int ln = 0;
    TokenizerState ts = init_tokenizer(buffer);
    while (1) {
        ln++;
        Token tok;
        tokens[0].t = '\n'; // TODO: this is a hack
        size_t num_tokens = 0;
        while (1) {
            tok = get_token(&ts);
            if (tok.t == '\n' || tok.t == TOK_EOF)
                break;
            if (num_tokens >= MAX_TOKENS_PER_LINE) {
                fprintf(stderr, "error: too many tokens on line %d\n", ln);
                exit(EXIT_FAILURE);
            }
            memcpy(&tokens[num_tokens], &tok, sizeof(Token));
            num_tokens++;
        }

        // labels
        if (tokens[0].t == TOK_IDENT) {
            if (num_tokens < 2 || tokens[1].t != ':') {
                fprintf(stderr, "parse error on line %d\n", ln);
                exit(EXIT_FAILURE);
            }
            add_symbol(tokens[0].s, curr_addr, ln);
            memmove(tokens, &tokens[2], sizeof(tokens[0])*(num_tokens-2));
            num_tokens -= 2;
        }

        if (tokens[0].t == TOK_MNEM) {
            curr_addr = parse_instr(tokens, num_tokens, output, curr_addr, ln);
        } else if (tokens[0].t == TOK_DIR) {
            if (strcmp(tokens[0].s, ".byte") == 0) {
                set_byte(output, curr_addr++, parse_int(tokens[1].s));
                // TODO: what if a .byte is followed by something wider?
            } else if (strcmp(tokens[0].s, ".half") == 0) {
                int n = parse_int(tokens[1].s);
                set_byte(output, curr_addr++, n & 0xff);
                set_byte(output, curr_addr++, (n >> 8) & 0xff);
            } else if (strcmp(tokens[0].s, ".word") == 0) {
                assert(curr_addr % 4 == 0);
                output[curr_addr/4] = parse_int(tokens[1].s);
                curr_addr += 4;
            } else if (strcmp(tokens[0].s, ".dword") == 0) {
                assert(curr_addr % 8 == 0);
                int64_t n = strtoll(tokens[1].s, NULL, 0);
                output[curr_addr/4] = n & 0xffffffff;
                curr_addr += 4;
                output[curr_addr/4] = (n >> 32) & 0xffffffff;
                curr_addr += 4;
            } else if (strcmp(tokens[0].s, ".string") == 0) {
                for (const char * cp = tokens[1].s+1; *cp != '"'; cp++) {
                    set_byte(output, curr_addr++, *cp);
                }
                set_byte(output, curr_addr++, '\0');
            } else {
                // TODO
            }
        } else if (tokens[0].t == '\n') {
        }

        num_words = (curr_addr+3)/4;

        if (tok.t == TOK_EOF)
            break;
    }

    resolve_refs(output);

    for (size_t i = 0; i < num_words; i++) {
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
strip_comments(char * s)
{
    /* replace comments with ' ' */
    /* TODO: use memmove to actually remove the comments instead of replacing with whitespace */
    size_t i;
    size_t l = strlen(s);
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
    strip_comments(file_contents.p);
    parse(file_contents);
    free(file_contents.p);

    return 0;
}
