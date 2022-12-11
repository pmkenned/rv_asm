    .text
    .globl _start
    .align 2
_start:
    c.nop
    c.add       a0, a1 
    c.addi      a0, 5
    c.addi16sp  sp, 16
    c.addi4spn  a0, sp, 4
    c.and       a0, a1
    c.andi      a0, 5
    c.beqz      a0, 8
    c.bnez      a0, 8
    c.ebreak
    c.j         8
    c.jalr      a0
    c.jr        a0
    c.li        a0, 5
    c.lui       a0, 5
    c.lw        a0, 8(a1)
    c.lwsp      a0, 8(sp)
    c.mv        a0, a1
    c.or        a0, a1
    c.slli      a0, 4
    c.srai      a0, 4
    c.srli      a0, 4
    c.sub       a0, a1
    c.sw        a0, 8(a1)
    c.swsp      a0, 8(sp)
    c.xor       a0, a1
