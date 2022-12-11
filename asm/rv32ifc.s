    .text
    .globl _start
    .align 2
_start:
# RV32FC only:
    c.flw       fa0, 4(a1)
    c.flwsp     fa0, 4(sp)
    c.fsw       fa0, 4(a1)
    c.fswsp     fa0, 4(sp)
