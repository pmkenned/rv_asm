    .text
    .align 2
    .globl _start
    .option rvc

_start:

#add     a0, a0, a1          # c.add         rd, rs2
#add     a0, x0, a1          # c.mv          rd, rs2
#
#addi    a0, x0, 4           # c.li          rd, imm
#addi    a0, a0, 4           # c.addi        rd, imm
#addi    sp, sp, 16          # c.addi16sp    imm
#addi    a0, sp, 4           # c.addi4spn    rd, imm
#
#and     a0, a0, a1          # c.and         rd, rs2
#
#andi    a0, a0, 4           # c.andi        rd, imm
#
#beq     a0, x0, 4           # c.beqz        rs1, offset
#
#ebreak                      # c.ebreak
#
#jal     x1, 8               # c.j offset
#jal     x1, 8               # c.jal offset
#
#jalr    x0, 0(a0)           # c.jr rs1
#jalr    x1, 0(a0)           # c.jalr rs1

lui     a0, 4               # c.lui   rd, imm
lui     a0, 0xfffff
lui     a0, 0xfffe0
lui     a0, 0x0001f

#sw          rs2, offset(rs1)
#    c.swsp  rs2, offset
#    c.sw    rs2, offset(rs1)

#slli        rd, rs1, shamt
#    c.slli  rd, shamt

#srai        rd, rs1, shamt
#    c.srai rd, shamt

#srli        rd, rs1, shamt
#    c.srli  rd, shamt

#sub         rd, rs1, rs2        [pseudo]
#    c.sub rd, rs2

#xor         rd, rs1, rs2
#    c.xor rd, rs2
#
#### RV32F and RV64F:
#flw         rd, offset(rs1)
#    c.flwsp rd, offset
#    c.flw   rd, offset(rs1)
#fsw         rs2, offset(rs1)
#    c.fswsp rs2, offset
#    c.fsw   rs2, offset(rs1)
#
#### RV32D and RV64D:
#fld         rd, offset(rs1)
#    c.fldsp rd, offset
#    c.fld   rd, offset(rs1)
#fsd         rs2, offset(rs1)
#    c.fsdsp rs2, offset
#    c.fsd   rs2, offset(rs1)
#
#### RV32IC and RV64IC:
#c.add       rd, rs2
#    add rd, rd, rs2
#c.addi      rd, imm
#    addi rd, rd, imm
#c.addi16sp  imm
#    addi x2, x2, imm            ; invalid when imm=0
#c.addi4spn  rd', imm
#    addi rd, x2, uimm where rd=8+rd'; invalid when imm=0
#c.and rd', rs2'
#    and rd, rd, rs2 where rd=8+rd' and rs2=8+rs2'
#c.andi rd', imm
#    andi rd, rd, imm where rd=8+rd'
#c.beqz rs1', offset
#    beq rs1, x0, offset where rs1=8+rs1'
#c.bnez rs1', offset
#    bne rs1, x0, offset where rs1=8+rs1'
#c.ebreak
#    ebreak
#c.j offset
#    jal x0, offset
#c.jalr  rs1
#    jalr x1, 0(rs1); invalid when rs1=x0
#c.jr    rs1
#    jalr x0, 0(rs1); invalid when rs1=x0
#c.li    rd, imm
#    addi rd, x0, imm
#c.lui   rd, imm
#    lui rd, imm; invalid when rd=x2 or imm=0
#c.lw    rd', uimm(rs1')
#    lw rd, uimm(rs1) where rd=8+rd' and rs1=8+rs1'
#c.lwsp  rd, uimm(x2)
#    lw rd, uimm(x2); invalid when rd=x0
#c.mv    rd, rs2
#    add rd, x0, rs2; invalid when rs2=x0
#c.or    rd', rs2'
#    or rd, rd, rs2 where rd=8+rd' and rs2=8+rs2'
#c.slli  rd, uimm
#    slli rd, rd, uimm
#c.srai  rd', uimm
#    srai rd, rd, uimm where rd=8+rd'
#c.srli  rd', uimm
#    srli rd, rd, uimm where rd=8+rd'
#c.sub   rd', rs2'
#    sub rd, rd, rs2 where rd=8+rd' and rs2=8+rs2'
#c.sw    rs2', uimm(rs1')
#    sw rs2, uimm(rs1) where rs2=8+rs2' and rs1=8+rs1'
#c.swsp rs2, uimm(x2)
#    sw rs2, uimm(x2)
#c.xor   rd', rs2'
#    xor rd, rd, rs2 where rd=8+rd' and rs2=8+rs2'
#
#### RV32IC only:
#c.jal   offset
#    jal x1, offset
#
#### RV32DC and RV64DC:
#c.fld   rd', uimm(rs1')
#    fld rd, uimm(rs1) where rd=8+rd' and rs1=8+rs1'
#c.fldsp rd', uimm(x2)
#    fld rd, uimm(x2)
#c.fsd   rd', uimm(rs1')
#    fsd rs2, uimm(rs1) where rs2=8+rs2' and rs1=8+rs1'
#c.fsdsp rd', uimm(x2)
#    fsd rs2, uimm(x2)
#
#### RV32FC only:
#c.flw   rd', uimm(rs1')
#    flw rd, uimm(rs1)  where rd=8+rd' and rs1=8+rs1'
#c.flwsp rd', uimm(x2)
#    flw rd, uimm(x2)
#c.fsw   rd', uimm(rs1')
#    fsw rs2, uimm(rs1) where rs2=8+rs2' and rs1=8+rs1'
#c.fswsp rd', uimm(x2)
#    fsw rs2, uimm(x2)
