#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>

#define NELEM(X) sizeof(X)/sizeof(X[0])

/* TODO
   * directives
   * pseudoinstructions
   * labels
   * output to file in binary
 */

/* TODO: quotation marks for strings */
typedef enum {
    TOK_NULL,
    TOK_DIR,
    TOK_MNEM,
    TOK_REG,
    TOK_COMMA,
    TOK_COLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_NEWLINE,
    TOK_NUMBER,
    TOK_IDENT,
    TOK_INVALID
} token_typ_t;

#if 0
const char * token_strs[] = {
    "NULL",
    "DIR",
    "MNEM",
    "REG",
    "COMMA",
    "COLON",
    "LPAREN",
    "RPAREN",
    "NEWLINE",
    "NUMBER",
    "IDENT",
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
    FMT_REG_OFFSET,
    FMT_REG_NUM,                // auipc rd,imm ; lui rd,imm ; li rd,imm
    FMT_REG_REG_REG,
    FMT_REG_REG_OFFSET,
    FMT_REG_REG_NUM,            // slli rd,rs1,shamt
    FMT_REG_NUM_REG,
    FMT_INVALID
} fmt_t;

static fmt_t
format_for_mnemonic(mnemonic_t mnemonic)
{
    switch (mnemonic) {
        case MNEM_LUI:      return FMT_NONE;                    break;
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
        case MNEM_CSRRW:    return FMT_REG_REG_REG;             break;
        case MNEM_CSRRS:    return FMT_REG_REG_REG;             break;
        case MNEM_CSRRC:    return FMT_REG_REG_REG;             break;
        case MNEM_CSRRWI:   return FMT_REG_REG_REG;             break;
        case MNEM_CSRRSI:   return FMT_REG_REG_REG;             break;
        case MNEM_CSRRCI:   return FMT_REG_REG_REG;             break;
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

#if 0
// mnem
    MNEM_EBREAK,
    MNEM_ECALL,
    MNEM_FENCE, // TODO: pred, succ
    MNEM_FENCE_I,

// mnem imm (jal label)
// TODO

// mnem reg, imm
    MNEM_LUI,
    MNEM_AUIPC,
    MNEM_JAL,

// mnem reg, reg, reg
    MNEM_ADD,
    MNEM_OR,
    MNEM_AND,
    MNEM_SUB,
    MNEM_XOR,
    MNEM_SLL,
    MNEM_SRL,
    MNEM_SRA,
    MNEM_SLT,
    MNEM_SLTU,

// mnem reg, reg, imm
    MNEM_BEQ,
    MNEM_BNE,
    MNEM_BLT,
    MNEM_BGE,
    MNEM_BLTU,
    MNEM_BGEU,
    MNEM_ADDI,
    MNEM_XORI,
    MNEM_ORI,
    MNEM_ANDI,
    MNEM_SLTI,
    MNEM_SLTIU,
    MNEM_SLLI,
    MNEM_SRLI,
    MNEM_SRAI,

// mnem reg, imm(reg)
    MNEM_JALR,
    MNEM_LB,
    MNEM_LH,
    MNEM_LW,
    MNEM_LBU,
    MNEM_LHU,
    MNEM_SB,
    MNEM_SH,
    MNEM_SW,

//    "csrrw",
//    "csrrs",
//    "csrrc",
//    "csrrwi",
//    "csrrsi",
//    "csrrci"
#endif 

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

/* TODO: decide if this should detect invalid register names */
static int
reg_name_to_bits(const char * reg_name)
{

    if (reg_name[0] == 'x') {
        int n = atoi(reg_name+1);
        assert(n >= 0 && n <= 31);
        return n;
    }

    if (reg_name[0] == 'a') {
        int n = atoi(reg_name+1);
        assert(n >= 0 && n <= 7);
        return n+10;
    }

    if (reg_name[0] == 's') {
        int n = atoi(reg_name+1);
        assert(n >= 0 && n <= 11);
        if (n <= 1)
            return n+8;
        else
            return n-2+18;
    }

    if (reg_name[0] == 't') {
        int n = atoi(reg_name+1);
        assert(n >= 0 && n <= 6);
        if (n <= 2)
            return n+5;
        else
            return n-3+28;
    }

    /* TODO: remove redundant stuff below */
    if (strcmp(reg_name, "x0") == 0) return 0;
    if (strcmp(reg_name, "x1") == 0 || strcmp(reg_name, "ra") == 0) return 1;
    if (strcmp(reg_name, "x2") == 0 || strcmp(reg_name, "sp") == 0) return 2;
    if (strcmp(reg_name, "x3") == 0 || strcmp(reg_name, "gp") == 0) return 3;
    if (strcmp(reg_name, "x4") == 0 || strcmp(reg_name, "tp") == 0) return 4;
    if (strcmp(reg_name, "x5") == 0 || strcmp(reg_name, "t0") == 0) return 5;
    if (strcmp(reg_name, "x6") == 0 || strcmp(reg_name, "t1") == 0) return 6;
    if (strcmp(reg_name, "x7") == 0 || strcmp(reg_name, "t2") == 0) return 7;
    if (strcmp(reg_name, "x8") == 0 || strcmp(reg_name, "s0") == 0 || strcmp(reg_name, "fp") == 0) return 8;
    if (strcmp(reg_name, "x9") == 0 || strcmp(reg_name, "s1") == 0) return 9;
    if (strcmp(reg_name, "x10") == 0 || strcmp(reg_name, "a0") == 0) return 10;
    if (strcmp(reg_name, "x11") == 0 || strcmp(reg_name, "a1") == 0) return 11;
    if (strcmp(reg_name, "x12") == 0 || strcmp(reg_name, "a2") == 0) return 12;
    if (strcmp(reg_name, "x13") == 0 || strcmp(reg_name, "a3") == 0) return 13;
    if (strcmp(reg_name, "x14") == 0 || strcmp(reg_name, "a4") == 0) return 14;
    if (strcmp(reg_name, "x15") == 0 || strcmp(reg_name, "a5") == 0) return 15;
    if (strcmp(reg_name, "x16") == 0 || strcmp(reg_name, "a6") == 0) return 16;
    if (strcmp(reg_name, "x17") == 0 || strcmp(reg_name, "a7") == 0) return 17;
    if (strcmp(reg_name, "x18") == 0 || strcmp(reg_name, "s2") == 0) return 18;
    if (strcmp(reg_name, "x19") == 0 || strcmp(reg_name, "s3") == 0) return 19;
    if (strcmp(reg_name, "x20") == 0 || strcmp(reg_name, "s4") == 0) return 20;
    if (strcmp(reg_name, "x21") == 0 || strcmp(reg_name, "s5") == 0) return 21;
    if (strcmp(reg_name, "x22") == 0 || strcmp(reg_name, "s6") == 0) return 22;
    if (strcmp(reg_name, "x23") == 0 || strcmp(reg_name, "s7") == 0) return 23;
    if (strcmp(reg_name, "x24") == 0 || strcmp(reg_name, "s8") == 0) return 24;
    if (strcmp(reg_name, "x25") == 0 || strcmp(reg_name, "s9") == 0) return 25;
    if (strcmp(reg_name, "x26") == 0 || strcmp(reg_name, "s10") == 0) return 26;
    if (strcmp(reg_name, "x27") == 0 || strcmp(reg_name, "s11") == 0) return 27;
    if (strcmp(reg_name, "x28") == 0 || strcmp(reg_name, "t3") == 0) return 28;
    if (strcmp(reg_name, "x29") == 0 || strcmp(reg_name, "t4") == 0) return 29;
    if (strcmp(reg_name, "x30") == 0 || strcmp(reg_name, "t5") == 0) return 30;
    if (strcmp(reg_name, "x31") == 0 || strcmp(reg_name, "t6") == 0) return 31;

    assert(0);
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

/* TODO */
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
    //10987654321098765432
    //wxxxxxxxxxxyzzzzzzzz
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
is_reg(const char * s)
{
    return str_in_list(s, reg_names, num_reg_names);
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
    ST_ERR
} state_t;

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

static state_t
common_next_state(char c)
{
    state_t next_state;
    if (isalpha(c) || c == '_') {
        next_state = ST_ALPHA;
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
    } else if (c == '\n') {
        next_state = ST_INIT;
    } else {
        next_state = ST_ERR;
    }
    return next_state;
}

char prev_token[1024];
char curr_token[1024];

static token_typ_t
next_char(char c)
{
    static size_t ti = 0;
    static state_t state = ST_INIT;
    state_t next_state = state;

    token_typ_t tok_typ = TOK_NULL;

    /* TODO: maybe emit the current token? */
    if (c == '\0') {
        state = ST_INIT;
        return TOK_NULL;
    }

    switch (state) {
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
            }
            if (next_state != ST_ALPHA) {
                if (is_reg(curr_token))
                    tok_typ = TOK_REG;
                else if (is_mnemonic(curr_token))
                    tok_typ = TOK_MNEM;
                else
                    tok_typ = TOK_IDENT;
            }
            break;
        case ST_LPAREN:
            next_state = common_next_state(c);
            tok_typ = TOK_LPAREN;
            break;
        case ST_RPAREN:
            next_state = common_next_state(c);
            tok_typ = TOK_RPAREN;
            break;
        case ST_NEWLINE:
            next_state = common_next_state(c);
            tok_typ = TOK_NEWLINE;
            break;
        case ST_COLON:
            next_state = common_next_state(c);
            tok_typ = TOK_COLON;
            break;
        case ST_COMMA:
            next_state = common_next_state(c);
            tok_typ = TOK_COMMA;
            break;
        case ST_PERIOD:
            if (isalnum(c)) {
                next_state = ST_DIR;
            } else {
                next_state = ST_ERR;
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
        ti = 0;
    }
    if (next_state != ST_INIT) {
        curr_token[ti++] = c;
        curr_token[ti] = '\0';
    }

    state = next_state;

    //printf("%c\t%s\t%s\n", c, state_strs[state], token_strs[tok_typ]);
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

/* TODO: this is bug-prone; in particular the use of static variables */
/* TODO: this involves excessive copying of return values */
static token_t
get_token(const char * buffer)
{
    token_t tok;
    token_typ_t t = TOK_NULL;
    static size_t i = 0;
    static size_t l;
    if (i == 0)
        l = strlen(buffer);
    while (i < l) {
        t = next_char(buffer[i++]);
        if (t != TOK_NULL)
            break;
    }
    tok.s[0] = '\0';
    tok.t = t;
    if (t != TOK_NULL)
        strcpy(tok.s, prev_token);
    return tok;
}

#if 0
static void
parse(const char * buffer)
{
    token_t t0;
    while (t0 = get_token(buffer), t0.t != TOK_NULL) {
        printf("%s (%s) ", token_strs[t0.t], t0.s);
    }
}
#endif

#if 0

typedef enum {
    PST_INIT,
    PST_I,
    PST_I_CL,
    PST_D,
    PST_D_N,
    PST_D_I,
    PST_M,
    PST_M_R,
//  PST_M_I,        // TODO: jal label
    PST_M_R_CM,
    PST_M_R_CM_I,
    PST_M_R_CM_R,
    PST_M_R_CM_R_CM,
    PST_M_R_CM_R_CM_I,
    PST_M_R_CM_R_CM_R,
    PST_M_R_CM_R_CM_N,
    PST_M_R_CM_N,
    PST_M_R_CM_N_LP,
    PST_M_R_CM_N_LP_R,
    PST_M_R_CM_N_LP_R_RP,
    PST_ERR
} parse_state_t;

#endif

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

#if 0
typedef enum {
    SEQ_LABEL,
    SEQ_DIR,
    SEQ_DIR_IDENT,
    SEQ_DIR_NUM,
    SEQ_MNEM,
    SEQ_MNEM_IDENT,
    SEQ_MNEM_REG_IDENT,
    SEQ_MNEM_REG_REG_REG,
    SEQ_MNEM_REG_REG_IDENT,
    SEQ_MNEM_REG_REG_NUM,
    SEQ_MNEM_REG_NUM_REG,
    SEQ_BLANK,
    SEQ_INVALID
} seq_type_t;

token_typ_t
valid_token_seqs[][10] = {
    {TOK_IDENT, TOK_COLON,      TOK_NEWLINE},
    {TOK_DIR,   TOK_NEWLINE},
    {TOK_DIR,   TOK_IDENT,      TOK_NEWLINE},
    {TOK_DIR,   TOK_NUMBER,     TOK_NEWLINE},
    {TOK_MNEM,  TOK_NEWLINE},
    {TOK_MNEM,  TOK_IDENT,      TOK_NEWLINE},
    {TOK_MNEM,  TOK_REG,        TOK_COMMA,      TOK_IDENT,  TOK_NEWLINE},
    {TOK_MNEM,  TOK_REG,        TOK_COMMA,      TOK_REG,    TOK_COMMA,      TOK_REG,    TOK_NEWLINE},
    {TOK_MNEM,  TOK_REG,        TOK_COMMA,      TOK_REG,    TOK_COMMA,      TOK_IDENT,  TOK_NEWLINE},
    {TOK_MNEM,  TOK_REG,        TOK_COMMA,      TOK_REG,    TOK_COMMA,      TOK_NUMBER, TOK_NEWLINE},
    {TOK_MNEM,  TOK_REG,        TOK_COMMA,      TOK_NUMBER, TOK_LPAREN,     TOK_REG,    TOK_RPAREN, TOK_NEWLINE},
    {TOK_NEWLINE}
};

const size_t num_valid_token_seqs = NELEM(valid_token_seqs);
#endif

#if 1
/* TODO: separate parsing from outputting */
static void
parse(const char * buffer)
{
//    size_t i, j;
    token_t tokens[10];
    token_t t0;
    size_t ti;
    int ln = 0;
    while (1) {
        ln++;
        ti = 0;
        tokens[0].t = TOK_NEWLINE; // TODO: this is a hack
        while (t0 = get_token(buffer), t0.t != TOK_NEWLINE && t0.t != TOK_NULL) {
            assert(ti < 10);
            memcpy(tokens+ti, &t0, sizeof(token_t));
            ti++;
            //printf("%s (%s) ", token_strs[t0.t], t0.s);
        }

        size_t num_tokens = ti;

#if 0
        for (i = 0; i < num_valid_token_seqs; i++) {
            const size_t n_max = NELEM(valid_token_seqs[0]);
            size_t n = 0;
            for (j = 0; j < n_max; j++) {
                if (valid_token_seqs[i][j] == TOK_NEWLINE)
                    break;
                n++;
            }
            if (ti == n) {
                int equal = 1;
                for (j = 0; j < ti; j++) {
                    if (tokens[j].t != valid_token_seqs[i][j]) {
                        equal = 0;
                        break;
                    }
                }
                if (equal) {
                    break;
                }
            }
        }

        seq_type_t seq_type = i;
#endif

        uint32_t opcode;
        if (tokens[0].t == TOK_MNEM) {

            mnemonic_t mnemonic = str_idx_in_list(tokens[0].s, mnemonics, num_mnemonics);

            assert(mnemonic != MNEM_INVALID); // TODO: error handling

            opcode = opcodes[mnemonic];

            fmt_t fmt = format_for_mnemonic(mnemonic);

            uint32_t rd, rs1, rs2, imm;
            uint32_t imm_fmt;

            switch (fmt) {
                case FMT_NONE:
                    break;

                case FMT_NONE_OR_IORW:
                    break;

                case FMT_REG_OFFSET_OR_OFFSET:
                    if (num_tokens == 4) {
                        rd = reg_name_to_bits(tokens[1].s);
                        if (tokens[3].t == TOK_NUMBER)
                            imm = atoi(tokens[3].s);
                        else
                            imm = 0; // TODO
                    } else if (num_tokens == 2) {
                        rd = reg_name_to_bits("x1");
                        if (tokens[1].t == TOK_NUMBER)
                            imm = atoi(tokens[1].s);
                        else
                            imm = 0; // TODO
                    }
                    imm_fmt = j_fmt_imm(imm);
                    opcode |= imm_fmt | (rd << 7);
                    break;

                case FMT_REG_OFFSET:
                    assert(0);
                    break;

                case FMT_REG_NUM:
                    assert(0);
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
                    else
                        imm = 0; // TODO
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

            printf("0x%08x\n", opcode);

        } else if (tokens[0].t == TOK_DIR) {
        } else if (tokens[0].t == TOK_IDENT) {
        } else if (tokens[0].t == TOK_NEWLINE) {
        }

#if 0
        switch (seq_type) {
            case SEQ_LABEL:
                /* TODO: add label to symbol tabel */
                break;
            case SEQ_DIR:
                /* TODO: interpret directive */
                break;
            case SEQ_DIR_IDENT:
                /* TODO: interpret directive */
                break;
            case SEQ_DIR_NUM:
                /* TODO: interpret directive */
                break;
            case SEQ_MNEM:
                printf("%08x\n", opcode);
                //if (strcmp(tokens[0].s, "ebreak") == 0) {
                //    printf("%08x\n", MNEM_EBREAK);
                //}
                break;
            case SEQ_MNEM_IDENT:
                printf("%08x\n", opcode);
                break;
            case SEQ_MNEM_REG_IDENT:
                printf("%08x\n", opcode);
                //if (strcmp(tokens[0].s, "jal") == 0) {
                //}
                break;
            case SEQ_MNEM_REG_REG_IDENT:
                printf("%08x\n", opcode);
                break;
            case SEQ_MNEM_REG_REG_REG:
                printf("%08x\n", opcode);
                break;
            case SEQ_MNEM_REG_REG_NUM:
                printf("%08x\n", opcode);
                break;
            case SEQ_MNEM_REG_NUM_REG:
                printf("%08x\n", opcode);
                break;
            case SEQ_BLANK:
                break;
            case SEQ_INVALID:
                /* TODO: handle errors */
                fprintf(stdout, "parsing error\n");
                exit(1);
                break;
            default:
                assert(0);
                break;
        }
#endif

        if (t0.t == TOK_NULL)
            break;
    }
}
#endif

int
main(int argc, char * argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
        exit(1);
    }

    FILE * fp = fopen(argv[1], "r");
    char buffer[1024];
    fread(buffer, 1, sizeof(buffer), fp);
    buffer[sizeof(buffer)-1] = '\0';
    fclose(fp);

    /* replace comments with ' ' */
    /* TODO: use memmove to actually remove the comments instead of replacing with whitespace */
    size_t i;
    size_t l = strlen(buffer);
    int comment_flag = 0;
    for (i = 0; i < l; i++) {
        if (!comment_flag) {
            if (buffer[i] == ';') {
                buffer[i] = ' ';
                comment_flag = 1;
            }
        } else {
            if (buffer[i] == '\n')
                comment_flag = 0;
            else
                buffer[i] = ' ';
        }
    }

    parse(buffer);

    return 0;
}
