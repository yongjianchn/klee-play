#!/bin/bash

#needed
sudo apt-get install g++ curl dejagnu subversion bison flex bc libcap-dev

#download llvm-gcc and put it to $PATH
if [ ! -f llvm-gcc4.2-2.9-x86_64-linux.tar.bz2 ]
then
	wget http://llvm.org/releases/2.9/llvm-gcc4.2-2.9-x86_64-linux.tar.bz2
fi
if [ ! -d llvm-gcc4.2-2.9-x86_64-linux ]
then
	tar xjvf llvm-gcc4.2-2.9-x86_64-linux.tar.bz2
fi
cd $(dirname $(find -type f -name llvm-gcc))
export PATH=$(pwd):${PATH}
cd -

#build llvm-2.9
if [ ! -f llvm-2.9.tgz ]
then
	wget http://llvm.org/releases/2.9/llvm-2.9.tgz
fi
if [ ! -d llvm-2.9 ]
then
	tar zxvf llvm-2.9.tgz
fi
#for gcc >= 4.7
sed -i '1 i#include <unistd.h>' llvm-2.9/lib/ExecutionEngine/JIT/Intercept.cpp
cd llvm-2.9
./configure --enable-optimized --enable-assertions \
&& make -j `grep -c processor /proc/cpuinfo`
if [ $? != 0 ]
then
	echo "llvm 2.9 --- failed..."
	exit 1
fi
cd $(dirname $(find -type f -name llvm-nm))
export PATH=$(pwd):${PATH}
cd -
cd ../

#build stp
if [ ! -f stp-r940.tgz ]
then
	wget http://www.doc.ic.ac.uk/~cristic/klee/stp-r940.tgz
fi
if [ ! -d stp-r940 ]
then
	tar xzfv stp-r940.tgz
fi
cd stp-r940
./scripts/configure --with-prefix=`pwd`/install --with-cryptominisat2 \
&& make OPTIMIZE=-O2 CFLAGS_M32= install -j `grep -c processor /proc/cpuinfo`
if [ $? != 0 ]
then
	echo "stp --- failed..."
	exit 1
fi
cd ../

ulimit -s unlimited

##build klee-uclibc
git clone --depth 1 --branch klee_0_9_29 https://github.com/klee/klee-uclibc.git
cd klee-uclibc/
./configure --make-llvm-lib \
&& make -j `grep -c processor /proc/cpuinfo`
if [ $? != 0 ]
then
	echo "klee-uclibc --- failed..."
	exit 1
fi
cd ../

#build klee
git clone https://github.com/ccadar/klee.git
cd klee
./configure --with-llvm=../llvm-2.9 --with-stp=../stp-r940/install --with-uclibc=../klee-uclibc --enable-posix-runtime \
&& make ENABLE_OPTIMIZED=1 -j `grep -c processor /proc/cpuinfo`
make #upline cmd may fail
if [ $? != 0 ]
then
	echo "klee --- failed..."
	exit 1
fi
cd -

echo "==================="
echo "编译完成，开始测试"
echo "==================="

cd klee
make check 
make unittests
cd -
echo "测试结束，请根据上述测试结果判断是否编译成功！"
echo "脚本结束！"
