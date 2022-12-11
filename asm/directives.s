.align 2
.globl _start
_start:
.section .rodata
.text
.bss
.data
.string "string"
.option rvc
.option norvc
.option pic
.option nopic
.option relax
.option norelax
.option push
.option pop

.align  2
.byte   4, 1, 2, 255
.half   1, 65535
.word   4, 0
.dword  4, 2
.balign 4
.zero   10

.equ    name, 3
.macro  foo arg1, argn
.endm
.float 3.14
.double 3.14159
