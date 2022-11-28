#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

void
die(const char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
	va_end(ap);
    exit(EXIT_FAILURE);
}

void
pack_le(void * p, size_t n, uint64_t x)
{
    assert(n <= 8);
    for (size_t i = 0; i < n; i++) {
        ((char *)p)[i] = x & 0xff;
        x >>= 8;
    }
}

uint64_t
unpack_le(void * p, size_t n)
{
    assert(n <= 8);
    uint64_t x = 0;
    for (size_t i = 0; i < n; i++) {
        x |= ((uint8_t *)p)[i] << 8*i;
    }
    return x;
}

/* return index of element in list if present; otherwise, return n */
size_t
str_idx_in_list(const char * str, const char * list[], size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        if (strcmp(str, list[i]) == 0)
            break;
    return i;
}

bool
str_in_list(const char * str, const char * list[], size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        if (strcmp(str, list[i]) == 0)
            return true;
    return false;
}

Buffer
read_file(const char * filename)
{
    FILE * fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    struct stat sb;
    if (fstat(fileno(fp), &sb) == -1) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    Buffer buffer;
    buffer.cap = (size_t) (sb.st_size);
    buffer.len = buffer.cap;
    buffer.p = malloc(sizeof(*buffer.p)*buffer.cap);
    fread(buffer.p, 1, buffer.len, fp);
    if (ferror(fp)) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    return buffer;
}

const char *
parse_int_strerror(int errnum)
{
    switch (errnum) {
        case 0: return NULL;
        case -1: return "not a valid number";
        case -2: return "extra characters at end of input";
        case -3: return "out of range of type long";
        case -4: return "greater than INT_MAX";
        case -5: return "less than INT_MIN";
        default: assert(0);
    }
    return NULL;
}

int
parse_int(const char * s, int * x) {
    char * end;
    errno = 0;
    const long sl = strtol(s, &end, 0);
    if (end == s)
        return -1;
    else if ('\0' != *end)
        return -2;
    else if ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        return -3;
    else if (sl > INT_MAX)
        return -4;
    else if (sl < INT_MIN)
        return -5;
    *x = (int) sl;
    return 0;
}

uint32_t
sext(uint32_t x, int w)
{
    assert(w <= 32);
    if (w == 32)
        return x;
    int msb = (x >> (w-1)) & 0x1;
    int mask = (1 << w) - 1;
    return msb ?
        x | ~mask :
        x & mask;
}
