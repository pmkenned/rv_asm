#ifndef TOKENIZER_H
#define TOKENIZER_H

extern char prev_token[1024];
extern char curr_token[1024];

/* TODO: quotation marks for strings */
typedef enum {
    TOK_NONE,
    TOK_DIR=128,
    TOK_MNEM,
    TOK_REG,
    TOK_CSR, // TODO
    TOK_NUMBER,
    TOK_IDENT,
    TOK_STRING, // TODO
    TOK_INVALID
} TokenType;

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
} State;

typedef struct {
    Buffer buffer;
    State state;
    size_t buf_pos;
    size_t tok_pos;
} TokenizerState;

TokenizerState init_tokenizer(Buffer buffer);

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

typedef struct {
    TokenType t;
    char s[1024];
} Token;

Token get_token(TokenizerState * ts);

#endif /* TOKENIZER_H */
