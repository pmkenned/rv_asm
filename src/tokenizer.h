#ifndef TOKENIZER_H
#define TOKENIZER_H

/* TODO: quotation marks for strings */
typedef enum {
    TOK_NONE,
    TOK_DIR=128,
    TOK_MNEM,
    TOK_PSEUDO,
    TOK_REG,
    TOK_CSR, // TODO
    TOK_NUM,
    TOK_IDENT,
    TOK_STRING, // TODO
    TOK_EOF,
    TOK_INVALID
} TokenType;

typedef enum {
    ST_INIT,
    ST_PERIOD,
    ST_DIR,
    ST_LPAREN,
    ST_RPAREN,
    ST_NEWLINE,
    ST_COLON,
    ST_COMMA,
    ST_MINUS,
    ST_DEC,
    ST_ZERO,
    ST_HEX,
    ST_OCT,
    ST_ALPHA,
    ST_DQUOTE,
    ST_CLOSE_QUOTE,
    ST_ERR
} State;

typedef struct {
    Buffer buffer;
    State state;
    int ln;
    bool eof;
    size_t buf_pos;
    bool emit_tok;
    size_t tok_begin;
    size_t tok_end;
} TokenizerState;

typedef struct {
    TokenType t;
    char s[64];
} Token;

TokenizerState init_tokenizer(Buffer buffer);
Token get_token(TokenizerState * ts);

#endif /* TOKENIZER_H */
