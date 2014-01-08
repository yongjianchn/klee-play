llvm-gcc -I../../klee/include --emit-llvm -c bench_test.c
#now we have bench_test.o
klee --libc=uclibc --posix-runtime --emit-all-errors bench_test.o --sym-args 2 2 3
ls -l klee-last/*.err

echo 
echo "many paths!!! fork in libc..."

