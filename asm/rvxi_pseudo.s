    .text
    .align 2
    .globl _start

# TODO: specify operands
_start:

beqz        rs1, offset         # beq rs1, x0, offset
bgez        rs1, offset         # bge rs1, x0, offset
bgt         rs1, rs2, offset    # blt rs2, rs1, offset
bgtu        rs1, rs2, offset    # bltu rs2, rs1, offset
bgtz        rs2, offset         # blt x0, rs2, offset
ble         rs1, rs2, offset    # bge rs2, rs1, offset
bleu        rs1, rs2, offset    # bgeu rs2, rs1, offset
blez        rs2, offset         # bge x0, rs2, offset
bltz        rs1, offset         # blt rs1, x0, offset
bnez        rs1, offset         # bne rs1, x0, offset
call        rd, symbol          # auipc rd, offsetHi; jalr rd, offsetLo(rd); if rd is omitted, x1
csrr        rd, csr             # csrrs rd, csr, x0
csrc        csr, rs1            # csrrc x0, csr, rs1
csrci       csr, zimm[4:0]      # csrrci x0, csr, zimm
csrs        csr, rs1            # csrrs x0, csr, rs1
csrsi       csr, zimm[4:0]      # csrrsi x0, csr, zimm
csrw        csr, rs1            # csrrw x0, csr, rs1
csrwi       csr, zimm[4:0]      # csrrwi x0, csr, zimm
j           offset              # jal x0, offset
jr          rs1                 # jalr x0, 0(rs1)
la          rd, symbol          # RV32I: auipc rd, offsetHi; lw rd, offsetLo(rd)
li          rd, imm             # RV32I: lui and/or addi
lla         rd, symbol          # auipc rd, offsetHi; addi rd, rd, offsetLo
mv          rd, rs1             # addi rd, rs1, 0
neg         rd, rs2             # sub rd, x0, rs2
nop                             # addi x0, x0, 0
not         rd, rs1             # xori rd, rs1, -1
rdcycle                         # csrrs rd, cycle, x0
rdinstret                       # csrrs rd, instret, x0
rdtime                          # csrrs rd, time, x0
ret                             # jalr x0, 0(x1)
seqz        rd, rs1             # sltiu rd, rs1, 1
sgtz        rd, rs2             # slt rd, x0, rs2
sltz        rd, rs1             # slt rd, rs1, x0
snez        rd, rs2             # sltu rd, x0, rs2
tail        symbol              # auipc x6, offsetHi; jalr x0, offsetLo(x6)
