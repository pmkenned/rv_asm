# rv_asm
RISC-V Assembler

## Notes

### RV32I and RV64I:
```
add     rd, rs1, rs2
    c.add       rd, rs2
    c.mv        rd, rs2
addi    rd, rs1, imm
    c.li        rd, imm
    c.addi      rd, imm
    c.addi16sp  imm
    c.addi4spn  rd, imm
and     rd, rs1, rs2
    c.and       rd, rs2
andi    rd, rs1, imm
    c.andi      rd, imm
auipc   rd, imm
beq     rs1, rs2, offset
    c.beq       rs1, offset
beqz    rs1, offset         [pseudo]
    beq rs1, x0, offset
bge     rs1, rs2, offset
bgeu    rs1, rs2, offset
bgez    rs1, offset         [pseudo]
    bge rs1, x0, offset
bgt     rs1, rs2, offset    [pseudo]
    blt rs2, rs1, offset
bgtu    rs1, rs2, offset    [pseudo]
    bltu rs2, rs1, offset
bgtz    rs2, offset         [pseudo]
    blt x0, rs2, offset
ble     rs1, rs2, offset    [pseudo]
    bge rs2, rs1, offset
bleu    rs1, rs2, offset    [pseudo]
    bgeu rs2, rs1, offset
blez    rs2, offset         [pseudo]
    bge x0, rs2, offset
blt     rs1, rs2, offset
bltz    rs1, offset         [pseudo]
    blt rs1, x0, offset
bltu    rs1, rs2, offset
bne     rs1, rs2, offset
bnez    rs1, offset         [pseudo]
    bne rs1, x0, offset
call    rd, symbol          [pseudo]
    auipc rd, offsetHi; jalr rd, offsetLo(rd); if rd is omitted, x1
csrr    rd, csr             [pseudo]
    csrrs rd, csr, x0
csrc    csr, rs1            [pseudo]
    csrrc x0, csr, rs1
csrci   csr, zimm[4:0]      [pseudo]
    csrrci x0, csr, zimm
csrrc   rd, csr, rs1
csrrci  rd, csr, zimm[4:0]
csrrs   rd, csr, rs1
csrrsi  rd, csr, zimm[4:0]
csrrw   rd, csr, rs1
csrrwi  rd, csr, zimm[4:0]
csrs    csr, rs1            [pseudo]
    csrrs x0, csr, rs1
csrsi   csr, zimm[4:0]      [pseudo]
    csrrsi x0, csr, zimm
csrw    csr, rs1            [pseudo]
    csrrw x0, csr, rs1
csrwi   csr, zimm[4:0]      [pseudo]
    csrrwi x0, csr, zimm
ebreak
ecall
fence   pred, succ; if args are omitted, iorw, iorw
fence.i
j       offset              [pseudo]
    jal x0, offset
jal     rd, offset; if rd is omitted, x1
    c.j offset
    c.jal offset
jalr    rd, offset(rs1); if rd is omitted, x1
    c.jr rs1
    c.jalr rs1
jr      rs1                 [pseudo]
    jalr x0, 0(rs1)
la      rd, symbol          [pseudo]
    RV32I: auipc rd, offsetHi; lw rd, offsetLo(rd)
    RV64I: auipc rd, offsetHi; ld rd, offsetLo(rd)
lb      rd, offset(rs1)
lbu     rd, offset(rs1)
lh      rd, offset(rs1)
lhu     rd, offset(rs1)
li      rd, imm             [pseudo]
    RV32I: lui and/or addi
    RV64I: lui, addi, slli, addi, slli, addi, slli, addi
lla     rd, symbol          [pseudo]
    auipc rd, offsetHi; addi rd, rd, offsetLo
lw      rd, offset(rs1)
lui     rd, imm
    c.lui   rd, imm
mret
mv      rd, rs1             [pseudo]
    addi rd, rs1, 0
neg     rd, rs2             [pseudo]
    sub rd, x0, rs2
nop                         [pseudo]
    addi x0, x0, 0
not     rd, rs1             [pseudo]
    xori rd, rs1, -1
or      rd, rs1, rs2
ori     rd, rs1, imm
rdcycle                     [pseudo]
    csrrs rd, cycle, x0
rdinstret                   [pseudo]
    csrrs rd, instret, x0
rdtime                      [pseudo]
    csrrs rd, time, x0
ret                         [pseudo]
    jalr x0, 0(x1)
sb      rs2, offset(rs1)
seqz    rd, rs1             [pseudo]
    sltiu rd, rs1, 1
sfence.vma  rs1, rs2
sgtz    rd, rs2             [pseudo]
    slt rd, x0, rs2
sh      rs2, offset(rs1)
sw      rs2, offset(rs1)
    c.swsp  rs2, offset
    c.sw    rs2, offset(rs1)
sll     rd, rs1, rs2
slli    rd, rs1, shamt
    c.slli  rd, shamt
slt     rd, rs1, rs2
slti    rd, rs1, imm
sltiu   rd, rs1, imm
sltu    rd, rs1, rs2
sltz    rd, rs1             [pseudo]
    slt rd, rs1, x0
snez    rd, rs2             [pseudo]
    sltu rd, x0, rs2
sra     rd, rs1, rs2
srai    rd, rs1, shamt
    c.srai rd, shamt
sret
srl     rd, rs1, rs2
srli    rd, rs1, shamt
    c.srli  rd, shamt
sub     rd, rs1, rs2        [pseudo]
    c.sub rd, rs2
tail    symbol              [pseudo]
    auipc x6, offsetHi; jalr x0, offsetLo(x6)
wfi
xor     rd, rs1, rs2
    c.xor rd, rs2
xori    rd, rs1, imm
```

### RV32I only:
```
rdcycleh                    [pseudo]
csrrs rd, cycleh, x0
rdinstreth                  [pseudo]
csrrs rd, instreth, x0
rdtimeh                     [pseudo]
csrrs rd, timeh, x0
```

### RV64I only:
```
addiw   rd, rs1, imm
    c.addiw     rd, imm
addw    rd, rs1, rs2
    c.addw      rd, rs2
ld      rd, offset(rs1)
    c.ldsp  rd, offset
    c.ld    rd, offset(rs1)
lwu     rd, offset(rs1)
negw    rd, rs2             [pseudo]
    subw rd, x0, rs2
sd      rs2, offset(rs1)
    c.sdsp  rs2, offset
    c.sd    rs2, offset(rs1)
sext.w  rd, rs1             [pseudo]
    addiw rd, rs1, 0
slliw   rd, rs1, shamt
sllw    rd, rs1, rs2
sraiw   rd, rs1, shamt
sraw    rd, rs1, rs2
srliw   rd, rs1, shamt
srlw    rd, rs1, rs2
subw    rd, rs1, rs2
    c.subw rd, rs2
```

### RV32M and RV64M:
```
div     rd, rs1, rs2
divu    rd, rs1, rs2
mul     rd, rs1, rs2
mulh    rd, rs1, rs2
mulhsu  rd, rs1, rs2
mulhu   rd, rs1, rs2
rem     rd, rs1, rs2
remu    rd, rs1, rs2
```

### RV64M only:
```
divuw   rd, rs1, rs2
divw    rd, rs1, rs2
mulw    rd, rs1, rs2
remuw   rd, rs1, rs2
remw    rd, rs1, rs2
```

### RV32F and RV64F:
```
fabs.s      rd, rs1
fadd.s      rd, rs1, rs2
fclass.s    rd, rs1, rs2
fcvt.s.w    rd, rs1, rs2
fcvt.s.wu   rd, rs1, rs2
fcvt.w.s    rd, rs1, rs2
fcvt.wu.s   rd, rs1, rs2
fdiv.s      rd, rs1, rs2
feq.s       rd, rs1, rs2
fle.s       rd, rs1, rs2
flt.s       rd, rs1, rs2
flw         rd, offset(rs1)
    c.flwsp rd, offset
    c.flw   rd, offset(rs1)
fmadd.s     rd, rs1, rs2, rs3
fmax.s      rd, rs1, rs2
fmin.s      rd, rs1, rs2
fmsub.s     rd, rs1, rs2, rs3
fmul.s      rd, rs1, rs2
fmv.w.x     rd, rs1, rs2
fmv.x.w     rd, rs1, rs2
fneg.s      rd, rs1         [pseudo]
    fsgnj.s rd, rs1, rs1
fnmadd.s    rs, rs1, rs2, rs3
fnmsubd.s   rs, rs1, rs2, rs3
frcsr       rd              [pseudo]
    csrrs   rd, fcsr, x0
frflags     rd              [pseudo]
    csrrs   rd, fflags, x0
frrm        rd              [pseudo]
    csrrs   rd, frm, x0
fscsr       rd, rs1         [pseudo]
    csrrw   rd, fcsr, rs1; if rd is omitted, x0
fsflags     rd, rs1         [pseudo]
    csrrw   rd, fflags; if rd is omitted, x0
fsgnj.s     rd, rs1, rs2
fsgnjn.s    rd, rs1, rs2
fsgnjx.s    rd, rs1, rs2
fsqrt.s     rd, rs1, rs2
fsrm        rd, rs1         [pseudo]
    csrrw   rd, frm, rs1; if rd is omitted, x0
fsub.s      rd, rs1, rs2
fsw         rs2, offset(rs1)
    c.fswsp rs2, offset
    c.fsw   rs2, offset(rs1)
```

### RV64F only:
```
fcvt.l.s    rd, rs1, rs2
fcvt.lu.s   rd, rs1, rs2
fcvt.s.l    rd, rs1, rs2
fcvt.s.lu   rd, rs1, rs2
```

### RV32D and RV64D:
fabs.d      rd, rs1
fadd.d      rd, rs1, rs2
fclass.d    rd, rs1, rs2
fcvt.d.s    rd, rs1, rs2
fcvt.d.w    rd, rs1, rs2
fcvt.d.wu   rd, rs1, rs2
fcvt.s.d    rd, rs1, rs2
fcvt.w.d    rd, rs1, rs2
fcvt.wu.d   rd, rs1, rs2
fdiv.d      rd, rs1, rs2
feq.d       rd, rs1, rs2
fld         rd, offset(rs1)
    c.fldsp rd, offset
    c.fld   rd, offset(rs1)
fle.d       rd, rs1, rs2
flt.d       rd, rs1, rs2
fmadd.d     rd, rs1, rs2, rs3
fmax.d      rd, rs1, rs2
fmin.d      rd, rs1, rs2
fmsub.d     rd, rs1, rs2, rs3
fmul.d      rd, rs1, rs2
fmv.d       rd, rs1         [pseudo]
    fsgnj.d rd, rs1, rs1
fneg.d      rd, rs1         [pseudo]
    fsgnj.d rd, rs1, rs1
fnmadd.d    rs, rs1, rs2, rs3
fnmsubd.d   rs, rs1, rs2, rs3
fsd         rs2, offset(rs1)
    c.fsdsp rs2, offset
    c.fsd   rs2, offset(rs1)
fsgnj.d     rd, rs1, rs2
fsgnjn.d    rd, rs1, rs2
fsgnjx.d    rd, rs1, rs2
fsqrt.d     rd, rs1, rs2
fsub.d      rd, rs1, rs2

### RV64D only:
```
fcvt.d.l    rd, rs1, rs2
fcvt.d.lu   rd, rs1, rs2
fcvt.l.d    rd, rs1, rs2
fcvt.lu.d   rd, rs1, rs2
fmv.d.x     rd, rs1, rs2
fmv.x.d     rd, rs1, rs2
```

### RV32IC and RV64IC:
```
c.add       rd, rs2
    add rd, rd, rs2
c.addi      rd, imm
    addi rd, rd, imm
c.addi16sp  imm
    addi x2, x2, imm; invalid when imm=0
c.addi4spn  rd', imm
    addi rd, x2, uimm where rd=8+rd'; invalid when imm=0
c.and
c.andi
c.beqz
c.bnez
c.ebreak
c.j
c.jalr
c.jr
c.li
c.lui
c.lw
c.lwsp
c.mv
c.or
c.slli
c.srai
c.srli
c.sub
c.sw
c.swsp
c.xor
```

### RV32IC only:
```
c.jal
```

### RV64IC only
```
c.addiw     rd, imm
    addiw rd, rd, imm; invalid when rd=x0
c.addw
c.ld
c.ldsp
c.sd
c.sdsp
c.subw
```

### RV32DC and RV64DC:
```
c.fld
c.fldsp
c.fsd
c.fsdsp
```

### RV32FC only:
```
c.flw
c.flwsp
c.fsw
c.fswsp
```

### RV32A and RV64A:
```
amoadd.w    rd, rs2, (rs1)
amoand.w    rd, rs2, (rs1)
amomax.w    rd, rs2, (rs1)
amomaxu.w   rd, rs2, (rs1)
amomin.w    rd, rs2, (rs1)
amominu.w   rd, rs2, (rs1)
amoor.w     rd, rs2, (rs1)
amoswap.w   rd, rs2, (rs1)
amoxor.w    rd, rs2, (rs1)
lr.w        rd, (rs1)
sc.w        rd, rs2, (rs1)
```

### RV64A only:
```
amoadd.d    rd, rs2, (rs1)
amoand.d    rd, rs2, (rs1)
amomax.d    rd, rs2, (rs1)
amomaxu.d   rd, rs2, (rs1)
amomin.d    rd, rs2, (rs1)
amominu.d   rd, rs2, (rs1)
amoor.d     rd, rs2, (rs1)
amoswap.d   rd, rs2, (rs1)
amoxor.d    rd, rs2, (rs1)
lr.d        rd, (rs1)
sc.d        rd, rs2, (rs1)
```
