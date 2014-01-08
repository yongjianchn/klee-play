#wget http://ftp.gnu.org/gnu/coreutils/coreutils-6.11.tar.gz
tar xzf coreutils-6.11.tar.gz
mkdir obj-gcov
cd obj-gcov
../coreutils-6.11/configure --disable-nls CFLAGS="-g -fprofile-arcs -ftest-coverage"
make -j4
make -C src arch hostname

cd src
rm *.gcda -f
./echo
gcov echo
cd ../
cd ../

mkdir obj-llvm
cd obj-llvm
../coreutils-6.11/configure --disable-nls CFLAGS="-g"
make CC=/home/xyj/research/klee/klee/scripts/klee-gcc
make -C src arch hostname CC=/home/xyj/research/klee/klee/scripts/klee-gcc

cd ../obj-llvm/src
echo "======>>>>>"
echo "start test echo.bc with --sym-arg 3"
sleep 10
klee --libc=uclibc --posix-runtime ./echo.bc --sym-arg 3
klee-stats klee-last
echo "======>>>>>"
echo "start test echo.bc with --optimize --sym-arg 3"
sleep 10
klee --optimize --libc=uclibc --posix-runtime ./echo.bc --sym-arg 3
klee-stats klee-last

cd ../../obj-gcov/src
echo "======>>>>>"
echo "start klee-replay ./echo with all.ktest"
sleep 10
klee-replay ./echo ../../obj-llvm/src/klee-last/*.ktest
gcov echo

cd ../../obj-llvm/src
echo "======>>>>>"
echo "start test echo.bc with --optimize --sym-args 0 2 4"
sleep 10
klee --only-output-states-covering-new --optimize --libc=uclibc --posix-runtime ./echo.bc --sym-args 0 2 4

cd ../../obj-gcov/src
m -f *.gcda
echo "======>>>>>"
echo "start klee-replay ./echo with all.ktest"
sleep 10
klee-replay ./echo ../../obj-llvm/src/klee-last/*.ktest
gcov echo
