#!/bin/bash
llvm-gcc -I/home/xyj/research/klee/klee/include --emit-llvm -c maze_klee.c
#now have maze_klee.o
echo ========================
echo ========================
echo ========================
echo ========================
echo ========================
echo ========================
echo ========================
echo "等下需要输入回车来继续"
echo ========================
echo ========================
echo ========================
klee --emit-all-errors maze_klee.o
ls -l klee-last/*err
