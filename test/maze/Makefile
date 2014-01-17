all:
	llvm-gcc -I/home/xyj/research/klee/klee/include --emit-llvm -c maze_klee.c
	klee -search=heuristic-path -stats --emit-all-errors maze_klee.o
	ls klee-last/*err*

clean:
	rm *.o
	rm klee-* -rf
