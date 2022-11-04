#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define NELEM(X) sizeof(X)/sizeof(X[0])

typedef struct {
    char * p;
    size_t n;
} Buffer;

extern const char * mnemonics[];
extern const size_t num_mnemonics;

Buffer read_file(const char * filename);

size_t str_idx_in_list(const char * str, const char * list[], size_t n);
#include "common.h"

int str_in_list(const char * str, const char * list[], size_t n);

#endif /* COMMON_H */
