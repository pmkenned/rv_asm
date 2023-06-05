#include "risc_v.h"

String pseudo_mnemonics[] = {
#define X(MNEM, STR, OPERANDS) CONST_STRING(STR),
    PSEUDO_LIST
#undef X
//    "invalid" // TODO: is this needed?
};

const size_t num_pseudo_mnemonics = NELEM(pseudo_mnemonics);

String mnemonics[] = {
#define X(MNEM, STR, FMT, OPERANDS, OPCODE) CONST_STRING(STR),
    INST_LIST
#undef X
};

const size_t num_mnemonics = NELEM(mnemonics);

uint32_t opcodes[] = {
#define X(MNEM, STR, FMT, OPERANDS, OPCODE) OPCODE,
    INST_LIST
#undef X
};

#define X CONST_STRING
String reg_names[] = {
    X("x0"),
    X("x1"),
    X("x2"),
    X("x3"),
    X("x4"),
    X("x5"),
    X("x6"),
    X("x7"),
    X("x8"),
    X("x9"),
    X("x10"),
    X("x11"),
    X("x12"),
    X("x13"),
    X("x14"),
    X("x15"),
    X("x16"),
    X("x17"),
    X("x18"),
    X("x19"),
    X("x20"),
    X("x21"),
    X("x22"),
    X("x23"),
    X("x24"),
    X("x25"),
    X("x26"),
    X("x27"),
    X("x28"),
    X("x29"),
    X("x30"),
    X("x31"),

    X("fp"), // same as s0
    X("ra"),
    X("sp"),
    X("gp"),
    X("tp"),
    X("t0"),
    X("t1"),
    X("t2"),
    X("s0"),
    X("s1"),
    X("a0"),
    X("a1"),
    X("a2"),
    X("a3"),
    X("a4"),
    X("a5"),
    X("a6"),
    X("a7"),
    X("s2"),
    X("s3"),
    X("s4"),
    X("s5"),
    X("s6"),
    X("s7"),
    X("s8"),
    X("s9"),
    X("s10"),
    X("s11"),
    X("t3"),
    X("t4"),
    X("t5"),
    X("t6")
};
#undef X

const size_t num_reg_names = NELEM(reg_names);
