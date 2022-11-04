#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

const char * mnemonics[] = {
    "lui",
    "auipc",
    "jal",
    "jalr",
    "beq",
    "bne",
    "blt",
    "bge",
    "bltu",
    "bgeu",
    "lb",
    "lh",
    "lw",
    "lbu",
    "lhu",
    "sb",
    "sh",
    "sw",
    "addi",
    "slti",
    "sltiu",
    "xori",
    "ori",
    "andi",
    "slli",
    "srli",
    "srai",
    "add",
    "sub",
    "sll",
    "slt",
    "sltu",
    "xor",
    "srl",
    "sra",
    "or",
    "and",
    "fence",
    "fence.i",
    "ecall",
    "ebreak",
    "csrrw",
    "csrrs",
    "csrrc",
    "csrrwi",
    "csrrsi",
    "csrrci"
};

const size_t num_mnemonics = NELEM(mnemonics);

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
    buffer.n = (size_t) (sb.st_size);
    buffer.p = malloc(sizeof(*buffer.p)*buffer.n);
    fread(buffer.p, 1, buffer.n, fp);
    if (ferror(fp)) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    return buffer;
}
