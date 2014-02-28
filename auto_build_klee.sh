#!/bin/bash

set -e
#set -x
JOBS="-j $(grep -c processor /proc/cpuinfo)"
gcc_version=`gcc --version|sed -n 1p|cut -d ' ' -f 4|cut -d '.' -f 2`
#needed
sudo apt-get install g++ curl dejagnu subversion bison flex bc libcap-dev

llvm_gcc_url=http://llvm.org/releases/2.9/llvm-gcc4.2-2.9-x86_64-linux.tar.bz2
llvm_2_9_url=http://llvm.org/releases/2.9/llvm-2.9.tgz
stp_url=http://www.doc.ic.ac.uk/~cristic/klee/stp-r940.tgz
klee_uclibc_git=https://github.com/klee/klee-uclibc.git
klee_git=http://github.com/klee/klee.git
zesti_url=http://srg.doc.ic.ac.uk/zesti/zesti.tar.gz

#download llvm-gcc and put it in $PATH
echo "setting llvm-gcc..."
if [ ! -d llvm-gcc4.2-2.9-x86_64-linux ]
then
	[ -f llvm-gcc4.2-2.9-x86_64-linux.tar.bz2 ] || wget $llvm_gcc_url
	tar xavf llvm-gcc4.2-2.9-x86_64-linux.tar.bz2 &> /dev/null
fi
cd $(dirname $(find -type f -name llvm-gcc))
	export PATH=$(pwd):${PATH}
cd -

#build llvm-2.9
echo "setting llvm-2.9..."
if [ ! -d llvm-2.9 ]
then
	[ -f llvm-2.9.tgz ] || wget ${llvm_2_9_url}
	tar xavf llvm-2.9.tgz &> /dev/null
fi
	#modify llvm-2.9 for gcc > 4.6
if [ $gcc_version -gt 6 ]
then
	sed -i '1 i#include <unistd.h>' llvm-2.9/lib/ExecutionEngine/JIT/Intercept.cpp
fi

cd llvm-2.9
	[ -f config.log ] || ./configure --enable-optimized --enable-assertions &> log.config
	make ${JOBS} &> log.make
		cd $(dirname $(find -type f -name llvm-nm))
			export PATH=$(pwd):${PATH}
		cd -
cd ../

#build stp
echo "setting stp-r940..."
if [ ! -d stp-r940 ]
then
        [ -f stp-r940.tgz ] || wget $stp_url
        tar xafv stp-r940.tgz &> /dev/null
fi
cd stp-r940
        [ -f config.log ] || ./scripts/configure --with-prefix=`pwd`/install --with-cryptominisat2 &> log.config
        make OPTIMIZE=-O2 CFLAGS_M32= install $JOBS &> log.make
cd ../

ulimit -s unlimited

##build klee-uclibc
echo "setting klee-uclibc..."
[ -d klee-uclibc ] || git clone --depth 1 --branch klee_0_9_29 ${klee_uclibc_git}
cd klee-uclibc/
	[ -f config.log ] || ./configure --make-llvm-lib &> log.config
	make ${JOBS} &> log.make
cd ../

#build klee
echo "setting klee..."
[ -d klee ] || git clone $klee_git
cd klee
	[ -f config.log ] || ./configure --with-llvm=../llvm-2.9 --with-stp=../stp-r940/install --with-uclibc=../klee-uclibc --enable-posix-runtime &> log.config
	make ENABLE_OPTIMIZED=1 ${JOBS} &> log.make
cd ../
echo "编译完成"
