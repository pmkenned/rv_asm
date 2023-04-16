#!/usr/bin/env bash

set -e

mkdir -p out
filename=$(basename -- "$1")
extension="${filename##*.}"
filename="${filename%.*}"

XLEN="${2:-32}"

MARCH=rv${XLEN}ifdc
if [[ $XLEN -eq "32" ]]
then
    MABI=ilp${XLEN}
elif [[ $XLEN -eq "64" ]]
then
    MABI=lp64
else
    echo "error: invalid XLEN"
    exit 1
fi

# compile and link
riscv64-unknown-elf-gcc -march=$MARCH -mabi=$MABI -c $1 -o out/$filename.o
#riscv64-unknown-elf-gcc -march=$MARCH -mabi=$MABI -Wl,--verbose -nostdlib out/$filename.o -o out/$filename
riscv64-unknown-elf-gcc -march=$MARCH -mabi=$MABI -Wl,-T scripts/linker_script.lds -nostdlib out/$filename.o -o out/$filename

# readelf
riscv64-unknown-elf-readelf -rseW out/$filename.o > out/$filename.o.readelf
riscv64-unknown-elf-readelf -rseW out/$filename > out/$filename.readelf

~/github/c/subprojects/elf/build/elfread out/$filename.o > out/$filename.o.readelf.mine
~/github/c/subprojects/elf/build/elfread out/$filename > out/$filename.readelf.mine

# disassemble
riscv64-unknown-elf-objdump -d -t -r out/$filename.o > out/$filename.o.dasm
riscv64-unknown-elf-objdump -d -t -r out/$filename > out/$filename.dasm
