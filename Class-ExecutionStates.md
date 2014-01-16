class ExecutionState 
===================
public成员变量
-------
* bool fakeState;
	* 不知道是什么
* unsigned underConstrained;
	* 不了解，只查到SpecialFunctionHandler.cpp中有以它做一些判断，但是未找到谁修改了它。
* unsigned depth;
	* state的分支深度。比较容易理解，root是0
* KInstIterator pc, prevPC;
	* pc和前一pc
* stack_ty stack;
	* stackFrame的vector类型，保存state的所有stackFrame（StackFrame类型）  
	其中StackFrame是一个struct, 包括caller，kfunction，callPathNode(其调用者的函数信息)，绑定的memoryObjects等内容。
* ConstrintManager constraints;
	* 约束集合
* mutable double queryCost;
	* 查询代价，用于search的state选择算法，和solver的判断和记录。
* double weight;
	* 权重，用于search选择新的state执行
* AddressSpace addressSpace;
	* 地址空间  
	AddressSpace是一个class，包含MemoryMap类型的成员。 
	MemoryMap:  
		`typedef ImmutableMap<const MemoryObject*, ObjectHolder, MemoryObjectLT> MemoryMap;`
* TreeOStream pathOS, symPathOS;
	* 用以记录分支路径，使用1和0代表true和false，比如state的path是111那么就是连续3次分支的true路径
* unsigned instsSinceCovNew;
	* 记录指令数，会在StatsTracker::stepInstruction()中如果识别到覆盖新的代码行数那么就改为1,并在每条指令执行的时候递增这个计数。也就是说记录了一个state从覆盖了新的代码到当前时刻这段时间执行的指令数。
* bool coveredNew;
	* 记录某个state是否覆盖了新的代码
* bool forkDisabled;
	* 是否允许fork，这个值由用户程序通过函数调用klee_set_forking()来控制
* std::map<const std::string*, std::set<unsigned> > coveredLines;
	* 记录了filename和line，每次遇到新的line，就修改。
* PTreeNode *ptreeNode;
	* processTree的指针类型，指向本state在processTree的位置。processTree是Executor的成员,用于结构化维护所有的state。暂时klee中用于方便地dump整个Tree，还用于RandomPathSearch::selectState()的实现。
* std::vector< std::pair<const MemoryObject*, const Array*> > symbolics;
	* 保存了所有的符号化的MemoryObject和对应的Array。MemoryObject主要保存address，size，id，ref等信息，Array主要保存Expr等信息。
* std::set<std::string> arrayNames;
	* 保存使用过的arrayNames，免得重复。
* MemoryMap shadowObjects;
	* 没有发现使用shadowObjects的地方。   
	注释说Used by the checkpoint/rollback methods for fake objects.
* unsigned incomingBBIndex;
	* 在transferToBasicBlock()函数中，state的pc被赋新值，之后判断pc对应指令是否是Instruction::PHI，如果是，那么就改变state.imcomingBBIndex为另一个BB的index, 用于下一条指令（新的pc是instruction::PHI）时使用。

public成员函数
-------
* ExecutionState(KFunction *kf);   
	* 除了会把*kf push 到stack中，别的成员变量采用一些默认初始值。
* ExecutionState(const std::vector<ref<Expr> > &assumptions);  
	* 除了会把constraints初始化为assumptions。别的一些默认值初始化
* ExecutionState(const ExecutionState& state);  
	* 相当于拷贝一份,同时把本state的symbolics中的MemoryObject的引用数++
* ~ExecutionState();  
	* 析构函数，主要是删除mo，pop stack
* std::string getFnAlias(std::string fn);
	* fnAliases是private成员，是一个map<string , string>；所以利用这个函数查找fn对应的别名。  
	  也就是返会map中第一个string是fn时对应的第二个string。
* void addFnAlias(std::string old_fn, std::string new_Fn);
	* 添加一个函数别名
* void removeFnAlias(std::string fn);
	* delete一个函数别名
* ExecutionState *branch()
	* depth++; 拷贝出一个newState，清除新的state的coveredNew和coveredLines；新旧state权重乘以0.5,然后返回这个新的state。
* void pushFrame(KInstIterator caller, KFunction *kf);
	* caller和kf建立一个StackFrame，然后push到stack中。
* void popFrame();
	* pop一个stackFrame，同时解除它绑定的MemoryObject。
* void addSymbolic(const MemoryObject *mo, const Array *array);
	* mo的引用次数加1, 然后make_pair(mo, array); push到symbolics
* void addConstraint(ref<Expr> e) {constraints.addConstraint(e);};
	* 调用constraints(ConstraintManager类型)的addConstraint函数，加入新的约束
* void dumpStack(std::ostream &out) const;
	* 遍历stack，把信息如函数名、函数参数名称和内容送到&out，如果指令info中包含文件名和行数，那么也送给&out。每次循环指令会去找stackFrame的caller从而得到caller指令信息。
* bool merge(const ExecutionState &b);
	* 作用是把&b这个state和本state能够merge起来。  
	能够merge需要：pc相同、symbolics相同、stack相同、addressSpace中的mo一一对应。  
	记录memoryObject相同objectHolder不同的情况并做一些操作（没看太懂）。  
	最后合并stackFrame、constraints。  
	constraintis的合并是(两个state的约束交集)and(A余下的约束orB余下的约束)
