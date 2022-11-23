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

int
str_in_list(const char * str, const char * list[], size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        if (strcmp(str, list[i]) == 0)
            return 1;
    return 0;
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

int
parse_int(const char * buff) {
  char *end;
  int si;
  errno = 0;
  const long sl = strtol(buff, &end, 0);
  if (end == buff) {
    fprintf(stderr, "%s: not a valid number\n", buff);
  } else if ('\0' != *end) {
    fprintf(stderr, "%s: extra characters at end of input: %s\n", buff, end);
  } else if ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno) {
    fprintf(stderr, "%s out of range of type long\n", buff);
  } else if (sl > INT_MAX) {
    fprintf(stderr, "%ld greater than INT_MAX\n", sl);
  } else if (sl < INT_MIN) {
    fprintf(stderr, "%ld less than INT_MIN\n", sl);
  } else {
    si = (int)sl;
    return si;
  }
  exit(EXIT_FAILURE);
}
