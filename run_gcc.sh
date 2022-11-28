#!/usr/bin/env bash

set -e

filename=$(basename -- "$1")
extension="${filename##*.}"
filename="${filename%.*}"
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -c $1 -o $filename.o
riscv64-unknown-elf-objdump -d $filename.o
