all:
	llvm-gcc -I/home/xyj/research/klee/klee/include --emit-llvm -c bench_test.c
	klee --libc=uclibc --posix-runtime --emit-all-errors bench_test.o --sym-args 2 2 5 
	ls klee-last/*err*

clean:
	rm *.o
	rm klee-* -rf
