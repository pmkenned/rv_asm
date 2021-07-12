#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define NELEM(X) sizeof(X)/sizeof(X[0])

/* TODO
   * parse token sequence
   * pseudoinstructions
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

#if 1
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

#if 1
static void
parse(const char * buffer)
{
    token_t tokens[10];
    token_t t0;
    size_t ti;
    while (1) {
        ti = 0;
        while (t0 = get_token(buffer), t0.t != TOK_NEWLINE && t0.t != TOK_NULL) {
            assert(ti < 10);
            memcpy(tokens+ti, &t0, sizeof(token_t));
            ti++;
            //printf("%s (%s) ", token_strs[t0.t], t0.s);

        }

        if (ti == 1 && tokens[0].t == TOK_DIR) {
            printf("directive\n");
        } else if (ti == 2 && tokens[0].t == TOK_DIR && tokens[1].t == TOK_IDENT) {
            printf("directive identifier\n");
        } else if (ti == 2 && tokens[0].t == TOK_DIR && tokens[1].t == TOK_NUMBER) {
            printf("directive number\n");
        } else if (ti == 2 && tokens[0].t == TOK_IDENT && tokens[1].t == TOK_COLON) {
            printf("label\n");
        } else if (ti == 1 && tokens[0].t == TOK_MNEM) {
            printf("mnemonic\n");
        } else if (ti == 4 && tokens[0].t == TOK_MNEM && tokens[1].t == TOK_REG && tokens[2].t == TOK_COMMA && tokens[3].t == TOK_IDENT) {
            printf("mnem reg, ident\n");
        } else if (ti == 6 && tokens[0].t == TOK_MNEM && tokens[1].t == TOK_REG && tokens[2].t == TOK_COMMA && tokens[3].t == TOK_REG && tokens[4].t == TOK_COMMA && tokens[5].t == TOK_IDENT) {
            printf("mnem reg, reg, ident\n");
        } else if (ti == 6 && tokens[0].t == TOK_MNEM && tokens[1].t == TOK_REG && tokens[2].t == TOK_COMMA && tokens[3].t == TOK_REG && tokens[4].t == TOK_COMMA && tokens[5].t == TOK_NUMBER) {
            printf("mnem reg, reg, number\n");
        } else if (ti == 6 && tokens[0].t == TOK_MNEM && tokens[1].t == TOK_REG && tokens[2].t == TOK_COMMA && tokens[3].t == TOK_REG && tokens[4].t == TOK_COMMA && tokens[5].t == TOK_REG) {
            printf("mnem reg, reg, reg\n");
        } else if (ti == 7 && tokens[0].t == TOK_MNEM && tokens[1].t == TOK_REG && tokens[2].t == TOK_COMMA && tokens[3].t == TOK_NUMBER && tokens[4].t == TOK_LPAREN && tokens[5].t == TOK_REG && tokens[6].t == TOK_RPAREN) {
            printf("mnem reg, num(reg)\n");
        } else if (ti == 0) {
            printf("blank\n");
        } else {
            printf("other: \n");
            size_t i;
            for (i=0; i<ti; i++) {
                printf("%s (%s) ", token_strs[tokens[i].t], tokens[i].s);
            }
            printf("\n");
        }

        //printf("\n");
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
