    .text
    .align 2
    .globl _start
    .option rvc

_start:

#add         rd, rs1, rs2
#    c.add       rd, rs2
#    c.mv        rd, rs2
#addi        rd, rs1, imm
#    c.li        rd, imm
#    c.addi      rd, imm
#    c.addi16sp  imm
#    c.addi4spn  rd, imm
#and         rd, rs1, rs2
#    c.and       rd, rs2
#andi        rd, rs1, imm
#    c.andi      rd, imm
#beq         rs1, rs2, offset
#    c.beqz      rs1, offset
#ebreak
#    c.ebreak
#jal         rd, offset          ; if rd is omitted, x1
#    c.j offset
#    c.jal offset
#jalr        rd, offset(rs1)     ; if rd is omitted, x1
#    c.jr rs1
#    c.jalr rs1
#lui         rd, imm
#    c.lui   rd, imm
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
#### RV64I only:
#addiw       rd, rs1, imm
#    c.addiw     rd, imm
#addw        rd, rs1, rs2
#    c.addw      rd, rs2
#ld          rd, offset(rs1)
#    c.ldsp  rd, offset
#    c.ld    rd, offset(rs1)
#sd          rs2, offset(rs1)
#    c.sdsp  rs2, offset
#    c.sd    rs2, offset(rs1)
#subw        rd, rs1, rs2
#    c.subw rd, rs2
#
#### RV64IC only
#c.addiw     rd, imm
#    addiw rd, rd, imm; invalid when rd=x0
#c.addw  rd', rs2'
#    addw rd, rd, rs2 where rd=8+rd' and rs2=8+rs2'
#c.ld    rd', uimm(rs1')
#    ld rd, uimm(rs1) where rd=8+rd' and rd1=8+rs1'
#c.ldsp  rd, uimm(x2)
#    ld rd, uimm(x2); invalid when rd=x0
#c.sd    rs2', uimm(rs1')
#    sd rs2, uimm(rs1) where rs2=8+rs2' and rs1=8+rs1'
#c.sdsp  rs2, uimm(x2)
#    sd rs2, uimm(x2)
#c.subw    rd', rs2'
#    subw rd, rd, rs2 where rd=8+rd' and rs2=8+rs2'
