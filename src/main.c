#include "common.h"
#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define MAX_TOKENS 10

typedef enum {
    MNEM_LUI,
    MNEM_AUIPC,
    MNEM_JAL,
    MNEM_JALR,
    MNEM_BEQ,
    MNEM_BNE,
    MNEM_BLT,
    MNEM_BGE,
    MNEM_BLTU,
    MNEM_BGEU,
    MNEM_LB,
    MNEM_LH,
    MNEM_LW,
    MNEM_LBU,
    MNEM_LHU,
    MNEM_SB,
    MNEM_SH,
    MNEM_SW,
    MNEM_ADDI,
    MNEM_SLTI,
    MNEM_SLTIU,
    MNEM_XORI,
    MNEM_ORI,
    MNEM_ANDI,
    MNEM_SLLI,
    MNEM_SRLI,
    MNEM_SRAI,
    MNEM_ADD,
    MNEM_SUB,
    MNEM_SLL,
    MNEM_SLT,
    MNEM_SLTU,
    MNEM_XOR,
    MNEM_SRL,
    MNEM_SRA,
    MNEM_OR,
    MNEM_AND,
    MNEM_FENCE,
    MNEM_FENCE_I,
    MNEM_ECALL,
    MNEM_EBREAK,
    MNEM_CSRRW,
    MNEM_CSRRS,
    MNEM_CSRRC,
    MNEM_CSRRWI,
    MNEM_CSRRSI,
    MNEM_CSRRCI,
    MNEM_INVALID,
} Mnemonic;

#if 1
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
        case MNEM_LUI:      return FMT_REG_NUM;                 break;
        case MNEM_AUIPC:    return FMT_REG_NUM;                 break;
        case MNEM_JAL:      return FMT_REG_OFFSET_OR_OFFSET;    break;
        case MNEM_JALR:     return FMT_REG_NUM_REG;             break;
        case MNEM_BEQ:      return FMT_REG_REG_OFFSET;          break;
        case MNEM_BNE:      return FMT_REG_REG_OFFSET;          break;
        case MNEM_BLT:      return FMT_REG_REG_OFFSET;          break;
        case MNEM_BGE:      return FMT_REG_REG_OFFSET;          break;
        case MNEM_BLTU:     return FMT_REG_REG_OFFSET;          break;
        case MNEM_BGEU:     return FMT_REG_REG_OFFSET;          break;
        case MNEM_LB:       return FMT_REG_NUM_REG;             break;
        case MNEM_LH:       return FMT_REG_NUM_REG;             break;
        case MNEM_LW:       return FMT_REG_NUM_REG;             break;
        case MNEM_LBU:      return FMT_REG_NUM_REG;             break;
        case MNEM_LHU:      return FMT_REG_NUM_REG;             break;
        case MNEM_SB:       return FMT_REG_NUM_REG;             break;
        case MNEM_SH:       return FMT_REG_NUM_REG;             break;
        case MNEM_SW:       return FMT_REG_NUM_REG;             break;
        case MNEM_ADDI:     return FMT_REG_REG_NUM;             break;
        case MNEM_SLTI:     return FMT_REG_REG_NUM;             break;
        case MNEM_SLTIU:    return FMT_REG_REG_NUM;             break;
        case MNEM_XORI:     return FMT_REG_REG_NUM;             break;
        case MNEM_ORI:      return FMT_REG_REG_NUM;             break;
        case MNEM_ANDI:     return FMT_REG_REG_NUM;             break;
        case MNEM_SLLI:     return FMT_REG_REG_NUM;             break;
        case MNEM_SRLI:     return FMT_REG_REG_NUM;             break;
        case MNEM_SRAI:     return FMT_REG_REG_NUM;             break;
        case MNEM_ADD:      return FMT_REG_REG_REG;             break;
        case MNEM_SUB:      return FMT_REG_REG_REG;             break;
        case MNEM_SLL:      return FMT_REG_REG_REG;             break;
        case MNEM_SLT:      return FMT_REG_REG_REG;             break;
        case MNEM_SLTU:     return FMT_REG_REG_REG;             break;
        case MNEM_XOR:      return FMT_REG_REG_REG;             break;
        case MNEM_SRL:      return FMT_REG_REG_REG;             break;
        case MNEM_SRA:      return FMT_REG_REG_REG;             break;
        case MNEM_OR:       return FMT_REG_REG_REG;             break;
        case MNEM_AND:      return FMT_REG_REG_REG;             break;
        case MNEM_FENCE:    return FMT_NONE_OR_IORW;            break;
        case MNEM_FENCE_I:  return FMT_NONE;                    break;
        case MNEM_ECALL:    return FMT_NONE;                    break;
        case MNEM_EBREAK:   return FMT_NONE;                    break;
        /* TODO: confirm csr* */
        case MNEM_CSRRW:    return FMT_REG_CSR_REG;             break;
        case MNEM_CSRRS:    return FMT_REG_CSR_REG;             break;
        case MNEM_CSRRC:    return FMT_REG_CSR_REG;             break;
        case MNEM_CSRRWI:   return FMT_REG_CSR_NUM;             break;
        case MNEM_CSRRSI:   return FMT_REG_CSR_NUM;             break;
        case MNEM_CSRRCI:   return FMT_REG_CSR_NUM;             break;
        case MNEM_INVALID:  assert(0);                          break;
        default:            assert(0);                          break;
    }
    assert(0);
    return FMT_INVALID;
}
#endif

uint32_t
opcodes[] = {
    0x00000037,
	0x00000017,
	0x0000006f,
	0x00000067,
	0x00000063,
	0x00001063,
	0x00004063,
	0x00005063,
	0x00006063,
	0x00007063,
	0x00000003,
	0x00001003,
	0x00002003,
	0x00004003,
	0x00005003,
	0x00000023,
	0x00001023,
	0x00002023,
	0x00000013,
	0x00002013,
	0x00003013,
	0x00004013,
	0x00006013,
	0x00007013,
	0x00001013,
	0x00005013,
	0x40005013,
	0x00000033,
	0x40000033,
	0x00001033,
	0x00002033,
	0x00003033,
	0x00004033,
	0x00005033,
	0x40005033,
	0x00006033,
	0x00007033,
	0x0000000f,
	0x0000100f,
	0x00000073,
	0x00100073,
	0x00001073,
	0x00002073,
	0x00003073,
	0x00005073,
	0x00006073,
	0x00007073
};

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
expect_n_tokens(int num_tokens, int n, int ln)
{
    if (num_tokens != n) {
        fprintf(stderr, "error: unexpected tokens on line %d\n", ln);
        exit(EXIT_FAILURE);
    }
}

static void
set_byte(uint32_t * output, size_t addr, size_t size, uint32_t data)
{
    uint32_t word = output[addr/4];
    int i = addr % 4;
    word &= ~(0xff << i*8);
    word |= (data & 0xff) << i*8;
    output[addr/4] = word;
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

    uint32_t output[1024]; // TODO
    size_t num_words = 0;

    uint32_t curr_addr = 0;

    Token tokens[MAX_TOKENS];
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
            if (num_tokens >= MAX_TOKENS) {
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

        uint32_t opcode;
        if (tokens[0].t == TOK_MNEM) {

            Mnemonic mnemonic = str_idx_in_list(tokens[0].s, mnemonics, num_mnemonics);
            if (mnemonic == MNEM_INVALID) {
                fprintf(stderr, "error: invalid mnemonic on line %d\n", ln);
                exit(EXIT_FAILURE);
            }
            opcode = opcodes[mnemonic];
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
                            imm = strtol(tokens[3].s, NULL, 0);
                    } else if (num_tokens == 2) {
                        rd = reg_name_to_bits("x1");
                        if (tokens[1].t == TOK_NUMBER)
                            imm = strtol(tokens[1].s, NULL, 0);
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
                    imm = strtol(tokens[3].s, NULL, 0);
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
                        imm = strtol(tokens[5].s, NULL, 0);
                    else {
                        add_ref(tokens[5].s, REF_B, curr_addr, ln);
                        imm = 0; // TODO
                    }
                    imm_fmt = b_fmt_imm(imm);
                    opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
                    break;

                case FMT_REG_REG_NUM:
                    expect_n_tokens(num_tokens, 6, ln);
                    rd = reg_name_to_bits(tokens[1].s);
                    rs1 = reg_name_to_bits(tokens[3].s);
                    imm = strtol(tokens[5].s, NULL, 0);
                    opcode |= (imm << 20) | (rs1 << 15) | (rd << 7);
                    break;

                case FMT_REG_NUM_REG:
                    expect_n_tokens(num_tokens, 7, ln);
                    imm = strtol(tokens[3].s, NULL, 0);
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

            assert(curr_addr/4 < NELEM(output));
            output[curr_addr/4] = opcode;
            //printf("%02x: 0x%08x\n", curr_addr, opcode);
            curr_addr += 4;
        } else if (tokens[0].t == TOK_DIR) {
            if (strcmp(tokens[0].s, ".byte") == 0) {
                //assert(is_num(tokens[1].s));
                set_byte(output, curr_addr++, 1, strtol(tokens[1].s, NULL, 0));
                // TODO: what if a .byte is followed by something wider?
            } else if (strcmp(tokens[0].s, ".half") == 0) {
                //assert(is_num(tokens[1].s));
                int n = strtol(tokens[1].s, NULL, 0);
                set_byte(output, curr_addr++, 1, n & 0xff);
                set_byte(output, curr_addr++, 1, (n >> 8) & 0xff);
            } else if (strcmp(tokens[0].s, ".word") == 0) {
                //assert(is_num(tokens[1].s));
                assert(curr_addr % 4 == 0);
                output[curr_addr/4] = strtol(tokens[1].s, NULL, 0);
                curr_addr += 4;
            } else if (strcmp(tokens[0].s, ".dword") == 0) {
                //assert(is_num(tokens[1].s));
                assert(curr_addr % 8 == 0);
                output[curr_addr/4] = (strtoll(tokens[1].s, NULL, 0) & 0x00000000ffffffff);
                curr_addr += 4;
                output[curr_addr/4] = (strtoll(tokens[1].s, NULL, 0) & 0xffffffff00000000) >> 32;
                curr_addr += 4;
            } else if (strcmp(tokens[0].s, ".string") == 0) {
                // TODO: add terminating null char
                for (const char * cp = tokens[1].s+1; *cp != '"'; cp++) {
                    set_byte(output, curr_addr++, 1, *cp);
                }
                set_byte(output, curr_addr++, 1, '\0');
            } else {
                // TODO
            }
        } else if (tokens[0].t == '\n') {
        }

        num_words = (curr_addr+3)/4;

        if (tok.t == TOK_EOF)
            break;
    }

    /* back-fill the references */
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

    for (size_t i = 0; i < num_words; i++) {
        printf("%08x\n", output[i]);
        //printf("%02lx: 0x%08x\n", i*4, output[i]);
    }

#if 0
    printf("\nsymbol table:\n");
    for (i = 0; i < num_symbols; i++) {
        printf("\t%s: %08x\n", symbols[i].s, symbols[i].addr);
    }
    printf("\nreferences:\n");
    for (i = 0; i < num_refs; i++) {
        printf("\t%s (%d): %02x\n", refs[i].s, refs[i].t, refs[i].addr);
    }
#endif
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
