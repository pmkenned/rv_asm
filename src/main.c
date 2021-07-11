#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define NELEM(X) sizeof(X)/sizeof(X[0])

/* TODO
   * Remove comments
   * split file into tokens
   * create struct for token with text
   * parse token sequence

valid forms:
   IDENT COLON
   DIR
   DIR NUM
   DIR IDENT
   MNEM REG COMMA IDENT
   MNEM REG COMMA REG COMMA IDENT
   MNEM REG COMMA REG COMMA REG
   MNEM REG COMMA REG COMMA NUM
   MNEM REG COMMA NUM LPAREN REG RPAREN
 */

typedef enum {
    TOK_NULL,
    TOK_DIR,
    TOK_MNEM,
    TOK_REG,
    TOK_COMMA,
    TOK_COLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_NUMBER,
    TOK_IDENT,
    TOK_INVALID,
    TOK_EOF
} token_t;

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
    "NUMBER",
    "IDENT",
    "INVALID",
    "EOF"
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
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "fp",
    "s1",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",
    "t3",
    "t4",
    "t5",
    "t6"
};

const size_t num_reg_names = NELEM(reg_names);

static int
is_dir(const char * t)
{
    size_t i;
    if (t[0] != '.')
        return 0;
    for (i = 0; i < num_directives; i++)
        if (strcmp(t, directives[i]) == 0)
            return 1;
    return 0;
}

static int
is_mnemonic(const char * t)
{
    size_t i;
    for (i = 0; i < num_mnemonics; i++)
        if (strcmp(t, mnemonics[i]) == 0)
            return 1;
    return 0;
}

static int
is_reg(const char * t)
{
    size_t i;
    for (i = 0; i < num_reg_names; i++)
        if (strcmp(t, reg_names[i]) == 0)
            return 1;
    return 0;
}

/* TODO: confirm */
static int
is_num(const char * t)
{
    size_t i, l;
    if (t[0] == '-')
        t++;
    l = strlen(t);
    for (i = 0; i < l; i++) {
        if (!isdigit(t[i]))
            return 0;
    }
    return 1;
}

static int
is_ident(const char * t)
{
    size_t i, l;
    if (t[0] != '_' && !isalpha(t[0]))
        return 0;
    t++;
    l = strlen(t);
    for (i = 0; i < l; i++) {
        if (t[i] != '_' && !isalnum(t[i]))
            return 0;
    }
    return 1;
}

static token_t
classify_token(const char * t)
{
    if (is_dir(t)) {
        return TOK_DIR;
    } else if (is_mnemonic(t)) {
        return TOK_MNEM;
    } else if (is_reg(t)) {
        return TOK_REG;
    } else if (strcmp(t, ",") == 0) {
        return TOK_COMMA;
    } else if (strcmp(t, ":") == 0) {
        return TOK_COLON;
    } else if (strcmp(t, "(") == 0) {
        return TOK_LPAREN;
    } else if (strcmp(t, ")") == 0) {
        return TOK_RPAREN;
    } else if (is_num(t)) {
        return TOK_NUMBER;
    } else if (is_ident(t)) {
        return TOK_IDENT;
    }
    return TOK_INVALID;
}

typedef enum {
    ST_INIT,
    ST_PERIOD,
    ST_DIR,
    ST_LPAREN,
    ST_RPAREN,
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
    "COLON",
    "COMMA",
    "DIGIT",
    "ALPHA",
    "ERR"
};

static state_t
common_next_state(c)
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

static token_t
next(char c)
{
    static size_t ti = 0;
    static state_t state = ST_INIT;
    state_t next_state = state;

    token_t tok = TOK_NULL;

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
            } else if (isspace(c)) {
                next_state = ST_INIT;
                tok = TOK_NUMBER;
            } else if (c == '(') {
                next_state = ST_LPAREN;
                tok = TOK_NUMBER;
            } else if (c == ')') {
                next_state = ST_RPAREN;
                tok = TOK_NUMBER;
            } else if (c == ',') {
                next_state = ST_COMMA;
                tok = TOK_NUMBER;
            } else if (c == ':') {
                next_state = ST_COLON;
                tok = TOK_NUMBER;
            } else {
                next_state = ST_ERR;
            }
            break;
        case ST_ALPHA:
            if (isalnum(c) || c == '_') {
                next_state = ST_ALPHA;
            } else if (isspace(c)) {
                next_state = ST_INIT;
            } else if (c == '(') {
                next_state = ST_LPAREN;
            } else if (c == ')') {
                next_state = ST_RPAREN;
            } else if (c == ',') {
                next_state = ST_COMMA;
            } else if (c == ':') {
                next_state = ST_COLON;
            } else {
                next_state = ST_ERR;
            }
            if (next_state != ST_ALPHA) {
                if (is_reg(curr_token))
                    tok = TOK_REG;
                else if (is_mnemonic(curr_token))
                    tok = TOK_MNEM;
                else
                    tok = TOK_IDENT;
            }
            break;
        case ST_LPAREN:
            next_state = common_next_state(c);
            tok = TOK_LPAREN;
            break;
        case ST_RPAREN:
            next_state = common_next_state(c);
            tok = TOK_RPAREN;
            break;
        case ST_COLON:
            next_state = common_next_state(c);
            tok = TOK_COLON;
            break;
        case ST_COMMA:
            next_state = common_next_state(c);
            tok = TOK_COMMA;
            break;
        case ST_PERIOD:
            if (isalnum(c)) {
                next_state = ST_DIR;
            } else {
                next_state = ST_ERR;
            }
            break;
        case ST_DIR:
            if (isspace(c)) {
                next_state = ST_INIT;
                tok = TOK_DIR;
            }
            break;
        case ST_ERR:
            fprintf(stdout, "error: invalid lex state\n");
            break;
        default:
            assert(0);
            break;
    }


    if (tok != TOK_NULL) {
        strcpy(prev_token, curr_token);
        ti = 0;
    }
    if (next_state != ST_INIT) {
        curr_token[ti++] = c;
        curr_token[ti] = '\0';
    }

    state = next_state;

    //printf("%c\t%s\t%s\n", c, state_strs[state], token_strs[tok]);
    if (tok != TOK_NULL) {
        printf("%s(%s) ", token_strs[tok], prev_token);
        //printf("%s ", prev_token);
    }
    if (c == '\n')
        printf("\n");

    return tok;
}


int
main(int argc, char * argv[])
{
#if 0
    if (argc < 2) {
        fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
        exit(1);
    }

    char ** lines;
    size_t lines_cap = 10;
    size_t num_lines = 0;
    lines = malloc(sizeof(*lines)*lines_cap);

    FILE * fp = fopen(argv[1], "r");
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        char * new_s = strdup(buffer);
        lines[num_lines] = new_s;
        num_lines++;
        if (num_lines >= lines_cap) {
            lines_cap *= 2;
            lines = realloc(lines, sizeof(*lines)*lines_cap);
        }
    }
    fclose(fp);

    size_t i, j;
    for (i = 0; i < num_lines; i++) {
        char * s = lines[i];

        j = 0;
        while (lines[i][j] != '\0') {
            if (lines[i][j] == ';') {
                lines[i][j] = '\0';
                break;
            }
            j++;
        }

        char * tok;
        while ((tok = strtok(s, " \n")) != NULL) {
            printf("%s(%s) ", tok, token_strs[classify_token(tok)]);
            s = NULL;
        }
        printf("\n");
        //printf("%s", lines[i]);
    }

    for (i = 0; i < num_lines; i++) {
        free(lines[i]);
    }
    free(lines);
#endif

    FILE * fp = fopen(argv[1], "r");
    char buffer[1024];
    fread(buffer, 1, sizeof(buffer), fp);
    buffer[sizeof(buffer)-1] = '\0';
    fclose(fp);

    /* replace comments with ' ' */
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

    for (i = 0; i < l; i++) {
        char c = buffer[i];
        token_t t = next(c);
        //printf("%c %s\n", c, token_strs[t]);
    }
    printf("\n");

    return 0;
}
