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

void die(const char * fmt, ...);
void pack_le(void * p, size_t n, uint64_t x);
uint64_t unpack_le(void * p, size_t n);
Buffer read_file(const char * filename);
size_t str_idx_in_list(const char * str, const char * list[], size_t n);
bool str_in_list(const char * str, const char * list[], size_t n);
const char * parse_int_strerror(int errnum);
int parse_int(const char * s, int * x);
uint32_t sext(uint32_t x, int w);

// defined in main.c
extern const char * mnemonics[];
extern const size_t num_mnemonics;
extern const char * pseudo_mnemonics[];
extern const size_t num_pseudo_mnemonics;

#endif /* COMMON_H */
