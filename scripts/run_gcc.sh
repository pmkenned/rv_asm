#!/usr/bin/env bash

set -e

mkdir -p out
filename=$(basename -- "$1")
extension="${filename##*.}"
filename="${filename%.*}"

MARCH=rv32ifdc
MABI=ilp32 
riscv64-unknown-elf-gcc -march=$MARCH -mabi=$MABI -c $1 -o out/$filename.o
riscv64-unknown-elf-gcc -march=$MARCH -mabi=$MABI -Wl,-T linker_script.lds -nostdlib out/$filename.o -o out/$filename
#riscv64-unknown-elf-objdump -d $filename.o
#riscv64-unknown-elf-objdump -d $filename

#MARCH=rv64ifdc
#MABI=lp64 
#riscv64-unknown-elf-gcc -march=$MARCH -mabi=$MABI -c $1 -o out/$filename.o
#riscv64-unknown-elf-gcc -Wl,-T linker_script.lds -nostdlib out/$filename.o -o out/$filename
##riscv64-unknown-elf-objdump -d $filename.o
##riscv64-unknown-elf-objdump -d $filename
