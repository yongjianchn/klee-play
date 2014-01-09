KLEE ANALYSIS
=============

##概览

**KLEE的主要组成部分**

* Interpreter
> * klee::Interpreter
> * klee::Executor
> * klee::ExecutionState
* 内存模型
* Expressions
* Searcher

##KLEE结构分析

###Namespace
Here is a list of namespaces
- ExprSMTLIBOptions 
- klee
	- bits32 有一些函数，主要是对bits进行值的比较和截断等等操作
	- bits64 同上
	- expr 有很多类，比如SMTParse、Identifier、Decl、ArrayDecl、VarDecl、Parser等
	- floats 定义了很多计算函数，基本都是uint64的加减乘除和比较操作等，这些函数都是先把操作数变成float类型再进行操作。此外还有Uint64AsDouble, Uint64AsFloat, DoubleAsUint64, FloatAsUint64等转换函数。
	- ImpliedValue 有函数getImpliedValue(), checkForImpliedValues()
	- ints 里面含有很多计算函数。以uint64类型直接操作，然后使用Namespace bits64下的截断函数返回uint64类型的值。
	- stats 有很多statistic类型的实例，比如Statistic allocations；Statistic forkTime；Statistic states等；可能是全局使用的一些东西，应该是在执行过程中管理的。
	- util 有两个struct 分别是ExprHash、Exprcmp， 还有两个函数getUserTime(), getWallTime().
- llvm 有static void AddStandardCompilePasses(); void Optimize();应该是用来添加llvm的pass。

###Classes
