    .text
    .globl _start
    .align 2
_start:
# RV32DC and RV64DC:
    c.fld       fa4, 0(a1)
    c.fldsp     fa4, 0(sp)
    c.fsd       fa4, 0(a1)
    c.fsdsp     fa4, 0(sp)
