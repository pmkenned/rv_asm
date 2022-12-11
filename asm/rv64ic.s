    .text
    .globl _start
    .align 2
_start:
# RV64IC only
    c.addiw     a0, 5
    c.addw      a0, a1
    c.ld        a0, 8(a1)
    c.ldsp      a0, 8(sp)
    c.sd        a0, 8(a1)
    c.sdsp      a0, 8(sp)
    c.subw      a0, a1
