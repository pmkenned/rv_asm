    .text
    .align 2
    .globl _start

    .option norvc

_start:
    addi a1,x0,5
    addi a0,x0,92
    jal x1,insert_sort    # jal insert_sort
    ebreak

insert_sort:
    addi a3,a0,4
    addi a4,x0,1
outer_loop:
    bltu a4,a1,continue #8
    jalr x0,0(ra)
continue:
    lw a6,0(a3)
    addi a2,a3,0
    addi a5,a4,0
inner_loop:
    lw a7,-4(a2)
    bge a6,a7,exit_inner #20
    sw a7,0(a2)
    addi a5,a5,-1
    addi a2,a2,-4
    bne a5,x0,inner_loop# -20
exit_inner:
    slli a5,a5,2
    add a5,a0,a5
    sw a6,0(a5)
    addi a4,a4,1
    addi a3,a3,4
    jal x0,outer_loop #-64

    .data
list:
    .word 4
    .word 3
    .word 7
    .word 2
    .word 5
#    .string "hello"
