#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define NELEM(X) sizeof(X)/sizeof(X[0])

typedef struct {
    char * p;
    size_t len;
    size_t cap;
} Buffer;

typedef struct {
    char * data;
    size_t len;
} String;

#define CONST_STRING(S) { .data = S, .len = sizeof(S)-1 }
#define STRING(S) ((String) CONST_STRING(S) )
#define LEN_DATA(S) (int) (S).len, (S).data

int string_equal(String s1, String s2);

void die(const char * fmt, ...);

void       pack_le(void * p, size_t n, uint64_t x);
uint64_t unpack_le(void * p, size_t n);

Buffer read_file(const char * filename);

const char * parse_int_strerror(int errnum);
int          parse_int(String str, int * x);

uint32_t sext(uint32_t x, int w);

#endif /* COMMON_H */
