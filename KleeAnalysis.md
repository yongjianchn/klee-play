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
- klee 含有一下namesapce以及部分typedef、非常多的Classes（后续分析）、10+全局变量如MaxCoreSolverTime、几十个全局函数（后续分析）。
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

根据klee官网的doxygen网页来帮助理解klee的代码结构。  
首先分析一下不在klee命名空间中的一些class。

* ValueRange 
对表达式可以做一些判断如是否是全集或者取表达式的最小值最大值等操作。

* ValidatingSolver 继承自klee::SolverImpl(TODO)  
根据名字和函数名如computeTruth()；computeValue()；可以推断其主要功能是对表达式进行合法性等求解。

* STPSolverImpl 继承自klee::SolverImpl  
可以设置求解时间、computeTruth、computeValue等功能

* SMTLIBLoggingSolver ：public QueryLoggingSolver ：public klee::SolverImpl  
用来将query以SMTLIBv2的形式放入到文件，应该不是分析的重点，并没有额外的public接口

* QueryLoggingSolver ： public klee::SolerImpl  
用于将qeury放入文件的抽象类，如SMTLIBLoggingSolver是其子类。  
还有一个子类是PCLoggingSoler，类似SMTLIBLoggingSoler，只是格式不同

* PPrinter 继承自 klee::ExprPPrinter  
用来打印表达式等。

* PrintContext  
 Helper class for pretty printing. It provides a basic wrapper around std::ostream that keeps track of how many characters have been used on the current line.  
 It also provides an optional way keeping track of the various levels of indentation by using a stack.

* KTest和KTestObject  
测试用例的struct，KTest用来保存一个测试用例，包括numArgs，args，symArgvs等属性，还包含KTestObject类型的指针。

* CachingSolver ： klee::SolverImpl  
有好几个solver，没弄清楚都是什么区别（TODO）

*还有一些类或者结构暂时感觉分析不清楚，而且觉得这些也不是最重要的。还是先搞清楚klee的主要执行流程，碰到重要的结构再去追究*

###分析klee工具的执行流程

**klee**

分析klee-src/tools/klee/main.cpp

* 初始化本地目标，使用llvm::INitializeNativeTarget(); 
* 传递参数 parseArguments(argc, argv);
* 如果设置了--watchdog并给出了--Max-time，那么就创建一个进程来用于kill(pid, SIGINT);
* sys::SetInterruptFunction(interrupt_handle);用于ctrl-c
* Load the bytecode to mainModule, if --posix-runtime is used, initEnv(mainModule).
* Set libc: NoLibc/KleeLibc/UcLibc
* Get the main function of mainModule.
* Get Argc\ArgV for target, set pEnvp(using the given file via --environ or system env).
* Running: two modes: Replay, Normal. 真正的执行函数：interpreter->runFunctionAsMain(mainFn, pArgc, pArgv, pEnvp); 需要看interpreter类
* Get statistics. 

分析runFunctionAsMain函数。

这个函数是Interpreper类的成员函数。以此为主线来分析klee的执行过程  
此处已有重要的数据结构：  

*handler = new KleeHandler(pArgc, pArgv);
Interpreter::InterpreterOptions IOpts;
Interpreter *interpreter = Interpreter::create(IOpts, handler);
handler->setInterpreter(interpreter);
interpreter 虽然是Interpreter指针类型，但是Interpreter::create()函数如下：
	Interpreter *Interpreter::create(const InterpreterOptions &opts,
			                                 InterpreterHandler *ih) {
		  return new Executor(opts, ih);
	}
返回一个Executor类型的指针。所以去追究Executor的runFunctionAsMain()函数。

Executor是Inpterpreter类的子类。是各种searcher类、tracker类的友元。  
分析执行过程从上述runFunctionAsMain()函数开始分析，其主要作用是创建初始state，初始化全局信息，创建processTree（该Tree的节点是执行状态state）。然后调用run()函数对初始state进行执行，等执行完毕，就做收尾工作（删除各种创建的东西）。

至此，执行过程中的重点落在Executor的run()函数上。对此函数分析如下：

	...种子模式省略
	
	  /*
	   *非种子模式下，使用searcher来符号执行每个状态
	   */
	  //get searcher, here we can use some new searcher. 
	  //To add searcher, modify Searcher.h & cpp, and UserSearcher.h & cpp
	  searcher = constructUserSearcher(*this);

	  //update(current, toinsert, toremove), here insert states and remove nothing
	  searcher->update(0, states, std::set<ExecutionState*>());

	  while (!states.empty() && !haltExecution) {
		ExecutionState &state = searcher->selectState();//具体的每个searcher类保存了一个states*队列，从这个队列中选择一个state返回
		KInstruction *ki = state.pc;//即将执行的状态是state，获取其pc
		stepInstruction(state);//用于统计或Track一些信息，比如统计所有的执行指令数

		executeInstruction(state, ki);//对当前state执行一条指令
		processTimers(&state, MaxInstructionTime);//启动一些timer来检查，timer应该是维护了指令级别的ticks，一条指令开始便开始tick，使用完毕ticks=0;
		//比如发现指令执行时间长于设定的最大时间，那么就terminate state

		/*
		 *针对设置了MaxMemory的情况来做一些操作
		 */
		if (MaxMemory) {
		  if ((stats::instructions & 0xFFFF) == 0) {//HOW？？？
			// We need to avoid calling GetMallocUsage() often because it
			// is O(elts on freelist). This is really bad since we start
			// to pummel the freelist once we hit the memory cap.
	#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
			unsigned mbs = sys::Process::GetMallocUsage() >> 20;
	#else
			unsigned mbs = sys::Process::GetTotalMemoryUsage() >> 20;
	#endif
			if (mbs > MaxMemory) {
			  if (mbs > MaxMemory + 100) {//如果使用的memory过多，那么kill一些states
				// just guess at how many to kill
				unsigned numStates = states.size();
				unsigned toKill = std::max(1U, numStates - numStates*MaxMemory/mbs);

				if (MaxMemoryInhibit)
				  klee_warning("killing %d states (over memory cap)",
							   toKill);

				std::vector<ExecutionState*> arr(states.begin(), states.end());
				for (unsigned i=0,N=arr.size(); N && i<toKill; ++i,--N) {
				  unsigned idx = rand() % N;

				  // Make two pulls to try and not hit a state that
				  // covered new code.
				  if (arr[idx]->coveredNew)
					idx = rand() % N;

				  std::swap(arr[idx], arr[N-1]);
				  terminateStateEarly(*arr[N-1], "Memory limit exceeded.");
				}
			  }
			  atMemoryLimit = true;
			} else {
			  atMemoryLimit = false;
			}
		  }
		}

		updateStates(&state);//包括searcher的states队列的更新和Executor的states set的更新
	  }


