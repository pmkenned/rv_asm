#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define NELEM(X) sizeof(X)/sizeof(X[0])

typedef struct {
    char * p;
    size_t n;
} Buffer;

Buffer read_file(const char * filename);
size_t str_idx_in_list(const char * str, const char * list[], size_t n);
int str_in_list(const char * str, const char * list[], size_t n);
int parse_int(const char * buff);

// defined in main.c
extern const char * mnemonics[];
extern const size_t num_mnemonics;

#endif /* COMMON_H */
