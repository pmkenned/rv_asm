#ifndef TOKENIZER_H
#define TOKENIZER_H

/* TODO: quotation marks for strings */
typedef enum {
    TOK_NONE,
    TOK_DIR=128,
    TOK_MNEM,
    TOK_PSEUDO,
    TOK_REG,
    TOK_FP_REG,
    TOK_CSR,
    TOK_NUM,
    TOK_IDENT,
    TOK_STRING,
    TOK_REL, // TODO
    TOK_EOF
} TokenType;

typedef struct {
    Buffer buffer;
    size_t pos;
    int ln;
    bool eof;
} Tokenizer;

typedef struct {
    TokenType type;
    String str;
} Token;

Tokenizer init_tokenizer(Buffer buffer);
Token get_token(Tokenizer * tz);

#endif /* TOKENIZER_H */
