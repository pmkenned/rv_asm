#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#define NELEM(X) sizeof(X)/sizeof(X[0])

typedef struct {
    char * p;
    size_t n;
} buffer_t;

/* TODO
   * directives:
        .text
        .data
        .bss
        .section
        .align
        .balign
        .globl
        .string
        .float
        .double
        .option
   * pseudoinstructions
   * output to file in binary
   * output ELF file
   * parse hex literals
   * string literals
   * %hi, %lo?
 */

/* TODO: quotation marks for strings */
typedef enum {
    TOK_NULL,
    TOK_DIR=128,
    TOK_MNEM,
    TOK_REG,
    TOK_CSR, // TODO
    TOK_NUMBER,
    TOK_IDENT,
    TOK_STRING, // TODO
    TOK_INVALID
} token_typ_t;

#if 0
const char * token_strs[] = {
    "NULL",
    "DIR",
    "MNEM",
    "REG",
    "CSR",
    "NUMBER",
    "IDENT",
    "STRING",
    "INVALID"
};
#endif

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
} mnemonic_t;

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
} fmt_t;

static fmt_t
format_for_mnemonic(mnemonic_t mnemonic)
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

const char * mnemonics[] = {
    "lui",
    "auipc",
    "jal",
    "jalr",
    "beq",
    "bne",
    "blt",
    "bge",
    "bltu",
    "bgeu",
    "lb",
    "lh",
    "lw",
    "lbu",
    "lhu",
    "sb",
    "sh",
    "sw",
    "addi",
    "slti",
    "sltiu",
    "xori",
    "ori",
    "andi",
    "slli",
    "srli",
    "srai",
    "add",
    "sub",
    "sll",
    "slt",
    "sltu",
    "xor",
    "srl",
    "sra",
    "or",
    "and",
    "fence",
    "fence.i",
    "ecall",
    "ebreak",
    "csrrw",
    "csrrs",
    "csrrc",
    "csrrwi",
    "csrrsi",
    "csrrci"
};

const size_t num_mnemonics = NELEM(mnemonics);

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

/* return index of element in list if present; otherwise, return n */
static size_t
str_idx_in_list(const char * str, const char * list[], ssize_t n)
{
    ssize_t i;
    for (i = 0; i < n; i++)
        if (strcmp(str, list[i]) == 0)
            break;
    return i;
}

const char * directives[] = {
    ".text",
    ".data",
    ".bss",
    ".section",
    ".align",
    ".balign",
    ".globl",
    ".string",
    ".byte",
    ".half",
    ".word",
    ".dword",
    ".float",
    ".double",
    ".option"
};

const size_t num_directives = NELEM(directives);

const char * reg_names[] = {
    "x0",
    "x1", "ra",
    "x2", "sp",
    "x3", "gp",
    "x4", "tp",
    "x5", "t0",
    "x6", "t1",
    "x7", "t2",
    "x8", "s0", "fp",
    "x9", "s1",
    "x10", "a0",
    "x11", "a1",
    "x12", "a2",
    "x13", "a3",
    "x14", "a4",
    "x15", "a5",
    "x16", "a6",
    "x17", "a7",
    "x18", "s2",
    "x19", "s3",
    "x20", "s4",
    "x21", "s5",
    "x22", "s6",
    "x23", "s7",
    "x24", "s8",
    "x25", "s9",
    "x26", "s10",
    "x27", "s11",
    "x28", "t3",
    "x29", "t4",
    "x30", "t5",
    "x31", "t6"
};

const size_t num_reg_names = NELEM(reg_names);

const char * csr_names[] = {
    "mstatus"
};

const size_t num_csr_names = NELEM(csr_names);

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

static int
str_in_list(const char * str, const char * list[], size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        if (strcmp(str, list[i]) == 0)
            return 1;
    return 0;
}

static int
is_mnemonic(const char * s)
{
    return str_in_list(s, mnemonics, num_mnemonics);
}

static int
is_num(const char * s)
{
    if (*s == '-')
        s++;
    if (*s == '\0')
        return 0;
    while (*s != '\0') {
        if (!isdigit(*s))
            return 0;
        s++;
    }
    return 1;
}

static int
is_reg(const char * s)
{
    return str_in_list(s, reg_names, num_reg_names);
}

static int
is_csr(const char * s)
{
    return str_in_list(s, csr_names, num_csr_names);
}

typedef enum {
    ST_INIT,
    ST_PERIOD,
    ST_DIR,
    ST_LPAREN,
    ST_RPAREN,
    ST_NEWLINE,
    ST_COLON,
    ST_COMMA,
    ST_DIGIT,
    ST_ALPHA,
    ST_DQUOTE,
    ST_CLOSE_QUOTE,
    ST_ERR
} state_t;

#if 0
const char * state_strs[] = {
    "INIT",
    "PERIOD",
    "DIR",
    "LPAREN",
    "RPAREN",
    "NEWLINE",
    "COLON",
    "COMMA",
    "DIGIT",
    "ALPHA",
    "ERR"
};
#endif

char prev_token[1024];
char curr_token[1024];

static state_t
common_next_state(char c)
{
    state_t next_state;
    if (isalpha(c) || c == '_') {
        next_state = ST_ALPHA;
    } else if (c == '"') {
        next_state = ST_DQUOTE;
    } else if (isdigit(c) || c == '-') {
        next_state = ST_DIGIT;
    } else if (c == '.') {
        next_state = ST_PERIOD;
    } else if (c == '(') {
        next_state = ST_LPAREN;
    } else if (c == ')') {
        next_state = ST_RPAREN;
    } else if (c == '\n') {
        next_state = ST_NEWLINE;
    } else if (c == ':') {
        next_state = ST_COLON;
    } else if (c == ',') {
        next_state = ST_COMMA;
    } else if (c == ' ') {
        next_state = ST_INIT;
    } else {
        fprintf(stderr, "prev_token: %s\n", prev_token);
        next_state = ST_ERR;
        //assert(0);
    }
    return next_state;
}

static token_typ_t
next_char(char c, state_t * state, size_t * token_pos)
{
    state_t next_state = *state;

    token_typ_t tok_typ = TOK_NULL;

    /* TODO: maybe emit the current token? */
    if (c == '\0') {
        *state = ST_INIT;
        return TOK_NULL;
    }

    switch (*state) {
        case ST_INIT:
            next_state = common_next_state(c);
            break;
        case ST_DIGIT:
            if (isdigit(c)) {
                next_state = ST_DIGIT;
            } else if (isblank(c)) {
                next_state = ST_INIT;
            } else if (c == '(') {
                next_state = ST_LPAREN;
            } else if (c == ')') {
                next_state = ST_RPAREN;
            } else if (c == '\n') {
                next_state = ST_NEWLINE;
            } else if (c == ',') {
                next_state = ST_COMMA;
            } else if (c == ':') {
                next_state = ST_COLON;
            } else {
                next_state = ST_ERR;
                assert(0);
            }
            if (next_state != ST_DIGIT)
                tok_typ = TOK_NUMBER;
            break;
        case ST_ALPHA:
            if (isalnum(c) || c == '_') {
                next_state = ST_ALPHA;
            } else if (isblank(c)) {
                next_state = ST_INIT;
            } else if (c == '(') {
                next_state = ST_LPAREN;
            } else if (c == ')') {
                next_state = ST_RPAREN;
            } else if (c == '\n') {
                next_state = ST_NEWLINE;
            } else if (c == ',') {
                next_state = ST_COMMA;
            } else if (c == ':') {
                next_state = ST_COLON;
            } else {
                next_state = ST_ERR;
                //assert(0);
            }
            if (next_state != ST_ALPHA) {
                if (is_reg(curr_token))
                    tok_typ = TOK_REG;
                else if (is_csr(curr_token))
                    tok_typ = TOK_CSR;
                else if (is_mnemonic(curr_token))
                    tok_typ = TOK_MNEM;
                else
                    tok_typ = TOK_IDENT;
            }
            break;
        case ST_DQUOTE:
            if (c == '"') {
                next_state = ST_CLOSE_QUOTE;
            } else if (c == '\\') {
                // TODO: support escape sequences
                assert(0);
            }
            break;
        case ST_CLOSE_QUOTE:
            next_state = common_next_state(c);
            tok_typ = TOK_STRING;
            break;
        case ST_LPAREN:
            next_state = common_next_state(c);
            tok_typ = '(';
            break;
        case ST_RPAREN:
            next_state = common_next_state(c);
            tok_typ = ')';
            break;
        case ST_NEWLINE:
            next_state = common_next_state(c);
            tok_typ = '\n';
            break;
        case ST_COLON:
            next_state = common_next_state(c);
            tok_typ = ':';
            break;
        case ST_COMMA:
            next_state = common_next_state(c);
            tok_typ = ',';
            break;
        case ST_PERIOD:
            if (isalnum(c)) {
                next_state = ST_DIR;
            } else {
                next_state = ST_ERR;
                assert(0);
            }
            break;
        case ST_DIR:
            if (isalpha(c)) {
                next_state = ST_DIR;
            } else if (isblank(c)) {
                next_state = ST_INIT;
            } else if (c == '\n') {
                next_state = ST_NEWLINE;
            } else {
                next_state = ST_ERR;
                assert(0);
            }
            if (next_state != ST_DIR)
                tok_typ = TOK_DIR;
            break;
        case ST_ERR:
            fprintf(stdout, "error: invalid lex state\n");
            break;
        default:
            assert(0);
            break;
    }


    if (tok_typ != TOK_NULL) {
        strcpy(prev_token, curr_token);
        *token_pos = 0;
    }
    if (next_state != ST_INIT) {
        curr_token[(*token_pos)++] = c;
        curr_token[*token_pos] = '\0';
    }

    *state = next_state;

    //printf("%c\t%s\t%s\n", c, state_strs[*state], token_strs[tok_typ]);
    if (tok_typ != TOK_NULL) {
        //printf("%s(%s) ", token_strs[tok_typ], prev_token);
        //printf("%s ", prev_token);
    }
    //if (c == '\n')
    //    printf("\n");

    return tok_typ;
}

typedef struct {
    token_typ_t t;
    char s[1024];
} token_t;

/* TODO: this involves excessive copying of return values */
static token_t
get_token(buffer_t buffer, size_t * pos, state_t * state, size_t * token_pos)
{
    token_t tok;
    token_typ_t t = TOK_NULL;
    while (*pos < buffer.n) {
        t = next_char(buffer.p[(*pos)++], state, token_pos);
        if (t != TOK_NULL)
            break;
    }
    tok.s[0] = '\0';
    tok.t = t;
    if (t != TOK_NULL)
        strcpy(tok.s, prev_token);
    return tok;
}

/*
TODO: lists for .byte, .half, etc. 

valid forms:
   IDENT COLON
   DIR
   DIR NUM
   DIR IDENT
   MNEM
   MNEM IDENT (pseudo-only, e.g. jal label)
   MNEM REG COMMA IDENT
   MNEM REG COMMA REG COMMA IDENT
   MNEM REG COMMA REG COMMA REG
   MNEM REG COMMA REG COMMA NUM
   MNEM REG COMMA NUM LPAREN REG RPAREN
*/

typedef struct {
    char * s;
    uint32_t addr;
} symbol_t;

// TODO
typedef enum {
    REF_J,
    REF_B
} ref_type_t;

typedef struct {
    char * s;
    ref_type_t t;
    uint32_t addr;
} ref_t;

/* TODO: separate parsing from outputting */
static void
parse(buffer_t buffer)
{

    // TODO: should be a dict
    size_t num_symbols = 0;
    size_t symbols_cap = 10;
    symbol_t * symbols = malloc(sizeof(*symbols)*symbols_cap);

    size_t num_refs = 0;
    size_t refs_cap = 10;
    ref_t * refs = malloc(sizeof(*refs)*refs_cap);

    uint32_t output[1024];
    size_t num_words = 0;

    uint32_t curr_addr = 0;

    size_t i;
    token_t tokens[10];
    token_t tok;
    size_t ti;
    int ln = 0;
    size_t pos = 0;
    state_t state = ST_INIT;
    size_t token_pos = 0;
    while (1) {
        ln++;
        ti = 0;
        tokens[0].t = '\n'; // TODO: this is a hack
        while (tok = get_token(buffer, &pos, &state, &token_pos), tok.t != '\n' && tok.t != TOK_NULL) {
            assert(ti < 10);
            memcpy(tokens+ti, &tok, sizeof(token_t));
            ti++;
            //printf("%s (%s) ", token_strs[tok.t], tok.s);
        }

        size_t num_tokens = ti;

        uint32_t opcode;
        if (tokens[0].t == TOK_MNEM) {

            mnemonic_t mnemonic = str_idx_in_list(tokens[0].s, mnemonics, num_mnemonics);

            assert(mnemonic != MNEM_INVALID); // TODO: error handling

            opcode = opcodes[mnemonic];

            fmt_t fmt = format_for_mnemonic(mnemonic);

            uint32_t rd, rs1, rs2, imm, imm_fmt;

            switch (fmt) {
                case FMT_NONE:
                    break;

                case FMT_NONE_OR_IORW:
                    /* TODO */
                    break;

                case FMT_REG_OFFSET_OR_OFFSET:
                    if (num_tokens == 4) {
                        rd = reg_name_to_bits(tokens[1].s);
                        if (tokens[3].t == TOK_NUMBER)
                            imm = atoi(tokens[3].s);
                    } else if (num_tokens == 2) {
                        rd = reg_name_to_bits("x1");
                        if (tokens[1].t == TOK_NUMBER)
                            imm = atoi(tokens[1].s);
                    }

                    if ((num_tokens == 2 && tokens[1].t == TOK_IDENT) ||
                        (num_tokens == 4 && tokens[3].t == TOK_IDENT)) {
                        refs[num_refs].s = strdup(tokens[3].s);
                        refs[num_refs].t = REF_J;
                        refs[num_refs].addr = curr_addr;
                        num_refs++;
                        if (num_refs >= refs_cap) {
                            refs_cap *= 2;
                            refs = realloc(refs, sizeof(*refs)*refs_cap);
                        }
                        imm = 0; // TODO
                    }

                    imm_fmt = j_fmt_imm(imm);
                    opcode |= imm_fmt | (rd << 7);
                    break;

                case FMT_REG_NUM:
                    rd  = reg_name_to_bits(tokens[1].s);
                    imm = atoi(tokens[3].s);
                    opcode |= u_fmt_imm(imm) | (rd << 7);
                    break;

                case FMT_REG_REG_REG:
                    rd  = reg_name_to_bits(tokens[1].s);
                    rs1 = reg_name_to_bits(tokens[3].s);
                    rs2 = reg_name_to_bits(tokens[5].s);
                    opcode |= (rs2 << 20) | (rs1 << 15) | (rd << 7);
                    break;

                case FMT_REG_REG_OFFSET:
                    rs1 = reg_name_to_bits(tokens[1].s);
                    rs2 = reg_name_to_bits(tokens[3].s);
                    if (tokens[5].t == TOK_NUMBER)
                        imm = atoi(tokens[5].s);
                    else {
                        refs[num_refs].s = strdup(tokens[5].s);
                        refs[num_refs].t = REF_B;
                        refs[num_refs].addr = curr_addr;
                        num_refs++;
                        if (num_refs >= refs_cap) {
                            refs_cap *= 2;
                            refs = realloc(refs, sizeof(*refs)*refs_cap);
                        }
                        imm = 0; // TODO
                    }
                    imm_fmt = b_fmt_imm(imm);
                    opcode |= imm_fmt | (rs2 << 20) | (rs1 << 15);
                    break;

                case FMT_REG_REG_NUM:
                    rd = reg_name_to_bits(tokens[1].s);
                    rs1 = reg_name_to_bits(tokens[3].s);
                    imm = atoi(tokens[5].s);
                    opcode |= (imm << 20) | (rs1 << 15) | (rd << 7);
                    break;

                case FMT_REG_NUM_REG:
                    imm = atoi(tokens[3].s);
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

                    break;

                case FMT_INVALID:
                    break;

                default:
                    //assert(0);
                    break;
            }

            assert(curr_addr/4 < NELEM(output));
            output[curr_addr/4] = opcode;
            //printf("%02x: 0x%08x\n", curr_addr, opcode);
            curr_addr += 4;
            num_words = curr_addr/4;

        } else if (tokens[0].t == TOK_DIR) {
            if (strcmp(tokens[0].s, ".byte") == 0) {
                assert(is_num(tokens[1].s));
                uint32_t word  = output[curr_addr/4];
                if (curr_addr % 4 == 0) {
                    word &= 0xffffff00;
                    word |= atoi(tokens[1].s) & 0xff;
                } else if (curr_addr % 4 == 1) {
                    word &= 0xffff00ff;
                    word |= (atoi(tokens[1].s) & 0xff) << 8;
                } else if (curr_addr % 4 == 2) {
                    word &= 0xff00ffff;
                    word |= (atoi(tokens[1].s) & 0xff) << 16;
                } else if (curr_addr % 4 == 3) {
                    word &= 0x00ffffff;
                    word |= (atoi(tokens[1].s) & 0xff) << 24;
                }
                output[curr_addr/4] = word;
                curr_addr += 1;
                num_words = curr_addr/4;
                // TODO: what if a .byte is followed by something wider?
            } else if (strcmp(tokens[0].s, ".half") == 0) {
                assert(is_num(tokens[1].s));
                uint32_t word  = output[curr_addr/4];
                if (curr_addr % 4 == 0) {
                    word &= 0xffff0000;
                    word |= atoi(tokens[1].s) & 0xffff;
                } else if (curr_addr % 4 == 2) {
                    word &= 0x0000ffff;
                    word |= (atoi(tokens[1].s) & 0xffff) << 16;
                } else {
                    assert(0);
                }
                output[curr_addr/4] = word;
                curr_addr += 2;
                num_words = curr_addr/4;
            } else if (strcmp(tokens[0].s, ".word") == 0) {
                assert(is_num(tokens[1].s));
                assert(curr_addr % 4 == 0);
                output[curr_addr/4] = atoi(tokens[1].s);
                curr_addr += 4;
                num_words = curr_addr/4;
            } else if (strcmp(tokens[0].s, ".dword") == 0) {
                assert(is_num(tokens[1].s));
                assert(curr_addr % 8 == 0);
                output[curr_addr/4] = (atoll(tokens[1].s) & 0x00000000ffffffff);
                curr_addr += 4;
                output[curr_addr/4] = (atoll(tokens[1].s) & 0xffffffff00000000) >> 32;
                curr_addr += 4;
                num_words = curr_addr/4;
            } else if (strcmp(tokens[0].s, ".string") == 0) {
                // TODO: this is buggy and shitty
                // TODO: add terminating null char
                const char * cp = tokens[1].s+1;
                size_t idx = 0;
                while (*cp != '"') {
                    uint32_t word  = output[curr_addr/4];
                    if (idx % 4 == 0) {
                        word &= 0xffffff00;
                        word |= *cp;
                    } else if (idx % 4 == 1) {
                        word &= 0xffff00ff;
                        word |= (uint32_t)(*cp) << 8;
                    } else if (idx % 4 == 2) {
                        word &= 0xff00ffff;
                        word |= (uint32_t)(*cp) << 16;
                    } else if (idx % 4 == 3) {
                        word &= 0x00ffffff;
                        word |= (uint32_t)(*cp) << 24;
                    }
                    idx++;
                    cp++;
                    output[curr_addr/4] = word;
                    curr_addr++;
                }
                // TODO: loses an incomplete word because of rounding down
                num_words = curr_addr/4;
                //printf("num_words: %zu\n", num_words);
            } else {
                // TODO
            }
        } else if (tokens[0].t == TOK_IDENT) {

            assert(num_tokens == 2 && tokens[1].t == ':'); // TODO: error checking

            // TODO: check if the symbol is already defined
            symbols[num_symbols].s = strdup(tokens[0].s);
            symbols[num_symbols].addr = curr_addr;
            num_symbols++;
            if (num_symbols >= symbols_cap) {
                symbols_cap *= 2;
                symbols = realloc(symbols, sizeof(*symbols)*symbols_cap);
            }

        } else if (tokens[0].t == '\n') {
        }

        if (tok.t == TOK_NULL)
            break;
    }

    /* back-fill the references */
    for (i = 0; i < num_refs; i++) {
        uint32_t word_idx = refs[i].addr/4;
        uint32_t opcode = output[word_idx];

        size_t j;
        int found = 0;
        for (j = 0; j < num_symbols; j++) {
            if (strcmp(symbols[j].s, refs[i].s) == 0) {
                found = 1;
                break;
            }
        }
        assert(found);
        uint32_t offset = symbols[j].addr - refs[i].addr;

        if (refs[i].t == REF_J) {
            opcode |= j_fmt_imm(offset);
        } else if (refs[i].t == REF_B) {
            opcode |= b_fmt_imm(offset);
        } else {
            // TODO: print error message, "referenced undefined symbol %s"
            assert(0);
        }
        output[word_idx] = opcode;
    }

    for (i = 0; i < num_words; i++) {
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

static buffer_t
read_file(const char * filename)
{
    FILE * fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    struct stat sb;
    if (fstat(fileno(fp), &sb) == -1) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    buffer_t buffer;
    buffer.n = (size_t) (sb.st_size+1);
    buffer.p = malloc(sizeof(*buffer.p)*buffer.n);
    fread(buffer.p, 1, buffer.n, fp);
    if (ferror(fp)) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    buffer.p[buffer.n-1] = '\0';
    return buffer;
}

int
main(int argc, char * argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
        exit(1);
    }

    buffer_t file_contents = read_file(argv[1]);
    strip_comments(file_contents.p);
    parse(file_contents);
    free(file_contents.p);

    return 0;
}
