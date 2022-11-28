#!/usr/bin/env bash

set -e

#MARCH=rv32ifdc
#MABI=ilp32 
MARCH=rv64ifdc
MABI=lp64 

filename=$(basename -- "$1")
extension="${filename##*.}"
filename="${filename%.*}"
riscv64-unknown-elf-gcc -march=$MARCH -mabi=$MABI -c $1 -o $filename.o
riscv64-unknown-elf-objdump -d $filename.o
