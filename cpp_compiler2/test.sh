#!/bin/bash
./bolgegc code.c code.asm
nasm -f elf64 -o code.o code.asm
ld -o program code.o
