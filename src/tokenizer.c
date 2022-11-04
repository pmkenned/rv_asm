#include "common.h"
#include "tokenizer.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>

char prev_token[1024];
char curr_token[1024];

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

const char * csr_names[] = {
    "mstatus"
};

const size_t num_csr_names = NELEM(csr_names);



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

static State
common_next_state(char c)
{
    State next_state;
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

static TokenType
next_char(char c, State * state, size_t * token_pos)
{
    State next_state = *state;

    TokenType tok_typ = TOK_NULL;

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
            // TODO: check directives list
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

/* TODO: this involves excessive copying of return values */
Token
get_token(Buffer buffer, size_t * pos, State * state, size_t * token_pos)
{
    Token tok;
    TokenType t = TOK_NULL;
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
