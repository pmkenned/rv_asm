#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>

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
int str_in_list(const char * str, const char * list[], size_t n);
int parse_int_or_die(const char * s);
int parse_int(const char * s, int * x);

// defined in main.c
extern const char * mnemonics[];
extern const size_t num_mnemonics;
extern const char * pseudo_mnemonics[];
extern const size_t num_pseudo_mnemonics;

#endif /* COMMON_H */
