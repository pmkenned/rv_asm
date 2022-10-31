    lui     x1, 0xfffff         # 20-bit imm: 0..1048575
    auipc   x2, 1048575         # 20-bit imm: 0..1048575
    jal     x3, 0x1abcde        # imm[20:1]
    jalr    x4, 4
    beq     x5, x6, 10
    bne     x6, x7, 6           # imm[12:1]
    blt     x8, x9, 128         # imm[12:1]
    bge     x10, x11, 0x10      # imm[12:1]
    bltu    x12, x13, 0x1ffe    # imm[12:1]
    bgeu    x14, x15, 2         # imm[12:1]
    lb      x16, 128(x17)       # imm[11:0]
    lh      x18, 127(x19)       # imm[11:0]
    lw      x20, 0x4bc(x21)     # imm[11:0]
    lbu     x22, 0x4bc(x23)     # imm[11:0]
    lhu     x24, 0x4bc(x25)     # imm[11:0]
    sb      x26, 0x4bc(x27)     # imm[11:0]
    sh      x28, 0x4bc(x29)     # imm[11:0]
    sw      x30, 0x4bc(x31)     # imm[11:0]
    addi    ra, sp, 0x7ff       # imm[11:0]
    slti    gp, tp, 2047
    sltiu   t0, t1, -0x800
    xori    t2, s0, -2048
    ori     s1, a0, 0x4bc
    andi    a1, a2, 0x4bc
    slli    a3, a4, 31
    srli    a5, a6, 31
    srai    a7, s2, 31
    add     s3, s4, s5
    sub     s6, s7, s8
    sll     s9, s10, s11
    slt     t3, t4, t5
    sltu    t6, x1, x2
    xor     x3, x4, x5
    srl     x5, x6, x7
    sra     x8, x9, x10
    or      x11, x12, x13
    and     x14, x15, x16
    fence
    fence.i
    ecall
    ebreak
    csrrw   x1, mhartid, x2
    csrrs   x2, mhartid, x3
    csrrc   x3, mhartid, x4
    csrrwi  x4, mhartid, 0x1f   # zimm[4:0]
    csrrsi  x5, mhartid, 31     # zimm[4:0]
    csrrci  x6, mhartid, 1      # zimm[4:0]
