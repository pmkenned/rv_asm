#include "common.h"
#include "tokenizer.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#if 1
const char * token_strs[] = {
    [TOK_NONE] = "NONE",
    [','] = ",",
    [':'] = ":",
    ['('] = ",",
    [')'] = ",",
    ['\n'] = "\\n",
    [TOK_DIR] = "DIR",
    [TOK_MNEM] = "MNEM",
    [TOK_PSEUDO] = "PSEUDO",
    [TOK_REG] = "REG",
    [TOK_CSR] = "CSR",
    [TOK_NUM] = "NUM",
    [TOK_IDENT] = "IDENT",
    [TOK_STRING] = "STRING",
    [TOK_EOF] = "EOF",
    [TOK_INVALID] = "INVALID"
};

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
    "DQUOTE",
    "CLOSE_QUOTE",
    "ERR"
};
#endif

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

TokenizerState
init_tokenizer(Buffer buffer)
{
    TokenizerState ts = {
        .buffer = buffer,
        .state = ST_INIT,
        .ln = 0,
        .eof = 0,
        .buf_pos = 0,
        .emit_tok = 0,
        .tok_begin = 0,
        .tok_end = 0
    };
    return ts;
}

static int
is_mnemonic(const char * s)
{
    return str_in_list(s, mnemonics, num_mnemonics);
}

static int
is_pseudo(const char * s)
{
    return str_in_list(s, pseudo_mnemonics, num_pseudo_mnemonics);
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
    } else if (c == '-') {
        next_state = ST_MINUS;
    } else if (c == '0') {
        next_state = ST_ZERO;
    } else if (c >= '1' && c <= '9') {
        next_state = ST_DEC;
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
        //fprintf(stderr, "prev_token: %s\n", prev_token);
        next_state = ST_ERR;
        //assert(0);
    }
    return next_state;
}


static TokenType
next_char(TokenizerState * ts)
{
    if (ts->buf_pos >= ts->buffer.n) {
        ts->tok_begin = 0;
        ts->tok_end = 0;
        return TOK_EOF;
    }
    char c = ts->buffer.p[ts->buf_pos];
    State next_state = ts->state;

    if (ts->emit_tok) {
        ts->tok_begin = ts->buf_pos-1;
    }

    TokenType tok_typ = TOK_NONE;

    /* TODO: maybe emit the current token? */
    if (c == '\0') {
        die("error: nul byte on line %d\n", ts->ln+1);
    }

    switch (ts->state) {
        case ST_INIT:
            next_state = common_next_state(c);
            break;
        case ST_MINUS:
            next_state = common_next_state(c);
            if (next_state == ST_MINUS)
                assert(0);
            break;
        case ST_ZERO:
            next_state = common_next_state(c);
            if (c == 'x' || c == 'X') {
                next_state = ST_HEX;
            } else if (c >= '0' && c <= '7') {
                next_state = ST_OCT;
            }
            if (next_state != ST_HEX && next_state != ST_OCT)
                tok_typ = TOK_NUM;
            break;
        case ST_DEC:
            if (isdigit(c)) {
                next_state = ST_DEC;
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
            if (next_state != ST_DEC)
                tok_typ = TOK_NUM;
            break;
        case ST_OCT:
            next_state = common_next_state(c);
            if (c >= '0' && c <= '7') {
                next_state = ST_OCT;
            }
            if (next_state != ST_OCT)
                tok_typ = TOK_NUM;
            break;
        case ST_HEX:
            next_state = common_next_state(c);
            c = tolower(c);
            if (isdigit(c) || (c >= 'a' && c <= 'f')) {
                next_state = ST_HEX;
            }
            if (next_state != ST_HEX)
                tok_typ = TOK_NUM;
            break;
        case ST_ALPHA:
            if (isalnum(c) || c == '_' || c == '.') {
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
                // TODO: this is an ugly hack
                char * curr_token = ts->buffer.p + ts->tok_begin;
                char t = ts->buffer.p[ts->buf_pos];
                ts->buffer.p[ts->buf_pos] = '\0';
                if (is_reg(curr_token))
                    tok_typ = TOK_REG;
                else if (is_csr(curr_token))
                    tok_typ = TOK_CSR;
                else if (is_mnemonic(curr_token))
                    tok_typ = TOK_MNEM;
                else if (is_pseudo(curr_token))
                    tok_typ = TOK_PSEUDO;
                else
                    tok_typ = TOK_IDENT;
                ts->buffer.p[ts->buf_pos] = t;
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
            ts->ln++;
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
            assert(0);
            break;
        default:
            assert(0);
            break;
    }

    if (tok_typ != TOK_NONE) {
        ts->tok_end = ts->buf_pos;
        ts->emit_tok = 1;
    } else {
        ts->emit_tok = 0;
    }

    if (ts->state == ST_INIT && next_state != ST_INIT) {
        assert(ts->emit_tok == 0);
        ts->tok_begin = ts->buf_pos;
    }

    ts->state = next_state;
    ts->buf_pos++;
    //printf("'%c'\t%s\t%s\n", c, state_strs[ts->state], token_strs[tok_typ]);
    return tok_typ;
}

/* TODO: this involves excessive copying of return values */
Token
get_token(TokenizerState * ts)
{
    Token tok;
    do  {
        tok.t = next_char(ts);
    } while (tok.t == TOK_NONE);
    size_t tok_len = ts->tok_end - ts->tok_begin;
    strncpy(tok.s, ts->buffer.p + ts->tok_begin, tok_len);
    tok.s[tok_len] = '\0';
    //printf("%s(%s) ", token_strs[tok.t], tok.s);
    return tok;
}
