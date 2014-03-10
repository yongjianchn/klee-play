/*
 * PathList.cpp -- Query and output all of the paths(based on basic block) 
 *                 between start point and end point
 */

#include "Passes.h"
#include <stdio.h>
#include <iostream>

using namespace llvm;
using namespace klee;

char PathList::ID = 0;

//static RegisterPass<PathList> X("pathlist", "search and output paths between start and end");
//INITIALIZE_PASS(PathList, "pathlist",
//              "search and output paths between start and end", false, false)
/*
ModulePass *llvm::createPathListPass(std::vector<std::vector<BasicBlock*> > &_paths, std::string _filename, int _lineno)
{
	PathList *pl = new PathList();
	pl->fileName = _filename;
	pl->lineNo = _lineno;
	pl->paths_found = _paths;
	return pl;
}
*/

PathList::PathList() : ModulePass(ID) {}

PathList::PathList(paths *_paths, std::string _filename, int _lineno) : ModulePass(ID) {
        this->fileName = _filename;
        this->lineNo = _lineno;
        this->paths_found = _paths;
	llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
	llvm::initializeBasicCallGraphPass(Registry);
	llvm::initializeCallGraphAnalysisGroup(Registry);                                 
	llvm::initializeDominatorTreePass(Registry);
}

static void getPath(SmallVectorImpl<char> &Path, const MDNode *MD) {
	if(MD==NULL)
		return;
	StringRef Filename = DIScope(MD).getFilename();
	if (sys::path::is_absolute(Filename))
		Path.append(Filename.begin(), Filename.end());
	else
		sys::path::append(Path, DIScope(MD).getDirectory(), Filename);
}

// get the Instruction in c code location
// addbyxqx201401
static std::string getInstPath(Instruction *I, unsigned &LineNo, unsigned &ColNo)
{
	MDNode *MD = I->getDebugLoc().getAsMDNode(I->getContext());
	if (!MD)
		return "";
	DILocation Loc(MD);
	SmallString<64> Path;
	getPath(Path, Loc.getScope());

	LineNo = Loc.getLineNumber();
	ColNo = Loc.getColumnNumber();
	std::string strPath = Path.str();
	return strPath;
	
}

bool PathList::findLineInBB(BasicBlock* BB, std::string srcFile, unsigned int srcLine)
{
	for (BasicBlock::iterator it = BB->begin(), ie = BB->end(); it != ie; ++it) {
		if (Instruction *I = dyn_cast<Instruction>(&*it)) {
			unsigned bbLine, bbCol;
			std::string bbFile = getInstPath(I, bbLine, bbCol);
			//std::cerr << " :checking " << bbFile << " : " << bbLine << "\n";
			if ((bbFile == srcFile) && (bbLine == srcLine)) {
				bug_Inst = I;
				return true;
			}
		}
	}		
	return false;
}

BasicBlock *PathList::getBB(std::string srcFile, int srcLine)
{
	for (Module::iterator fit=module->begin(); fit != module->end(); ++fit) {
		Function *F = fit;
		for (Function::iterator bbit=F->begin(); bbit != F->end(); ++bbit) {
			BasicBlock *BB = bbit;
			if (findLineInBB(BB, srcFile, srcLine))
				return BB;
		}
	}
	return NULL;
}

static BasicBlock *findCommonDominator(BasicBlock *BB, DominatorTree *DT) {
	pred_iterator i = pred_begin(BB), e = pred_end(BB);
	BasicBlock *Dom = *i;
	for (++i; i != e; ++i)
		Dom = DT->findNearestCommonDominator(Dom, *i);
	return Dom;
}

bool PathList::isBackEdge(llvm::BasicBlock *From, llvm::BasicBlock *To) {
	return std::find(BackEdges.begin(), BackEdges.end(), Edge(From, To))
		!= BackEdges.end();
}

void PathList::printCalledFuncAndCFGPath(Function *srcFunc, Function *dstFunc, BasicBlock *BB, std::vector<BasicBlock*> *p){
	CalledFunctions::iterator ii, ee;
	ii = calledFunctionMap[srcFunc].begin();
	ee = calledFunctionMap[srcFunc].end();
	for( ; ii != ee; ++ii){
		
		Function *FTmp = ii->first;
		Instruction *ITmp = ii->second;
		

		llvm::errs() << "\t" << FTmp->getName() << "-->" ;
		llvm::errs() << "\n" << *ITmp <<"\n";

		BasicBlock *curBB = ITmp->getParent();
//		BasicBlock *curBB = BB;
		pred_iterator i, e =  pred_end(curBB);
		
		DT = &getAnalysis<DominatorTree>(*FTmp);
		BackEdges.clear();
		if( FTmp ){
			FindFunctionBackedges(*FTmp, BackEdges);
			//Backedges = &BackEdges;
		}

		llvm::errs() << "\t\tCFG in :" << FTmp->getName() << "\n\t\t" << curBB->getName() << "->";
		
		//output the path of bug function
		if (bug_Inst->getParent() == BB) {
			p->insert(p->begin(), BB);
			for (i = pred_begin(BB); i != pred_end(BB); ) {
				BasicBlock *tmpb = *i;
				errs()<<"\t"<<tmpb->getName()<<"->";
				p->insert(p->begin(), tmpb);
				i = pred_begin(tmpb);
				if (i != pred_end(BB) && isBackEdge(*i, tmpb)) {
					tmpb = findCommonDominator(tmpb, &getAnalysis<DominatorTree>(*(bug_Inst->getParent()->getParent())));
					llvm::errs() << "\tbk:" << tmpb->getName() << "->";
					p->insert(p->begin(), tmpb);
					i = pred_begin(tmpb);
				}
			}
		}
		
		p->insert(p->begin(), curBB);
		for (i = pred_begin(curBB); i != e; )
		{

			BasicBlock *bb = *i;
			//bb->getName();
			llvm::errs() << "\t" << bb->getName() << "->";
			p->insert(p->begin(), bb);
			i = pred_begin(bb);
			if(i!=e && isBackEdge(*i,bb))
			{
				bb = findCommonDominator(bb, DT);
				llvm::errs() << "\tbk:" << bb->getName() << "->";
				p->insert(p->begin(), bb);
				i = pred_begin(bb);
				//break;
			}
		}
		llvm::errs() << "\n\t\tCFG end\n" ;
		if( FTmp == dstFunc ){
			llvm::errs() << "end. \na path found\n\n";
			paths_found->insert(paths_found->begin(), *p);
			p->clear();
			break;
		}
		else
		{
			printCalledFuncAndCFGPath(FTmp, dstFunc, curBB, p);
		}
	}
	
}

bool PathList::runOnModule(Module &M) {
	module = &M;

	CallGraph &CG = getAnalysis<CallGraph>();
	//CG.dump();
	
	CallGraphNode *cgNode = CG.getRoot();
	//cgNode->dump();
//	errs()<<node->getFunction()->getName()<<'\n';

	Function *startFunc;
	Function *endFunc;
	startFunc = cgNode->getFunction();
//	endFunc = M.getFunction("list_entries_users");
	
	//std::string fileName("/home/wanghuan/research/others/countlines/coreutils-8.22/src/who.c");
//	int lineNo = 195;
	std::cerr << "xyj---fileName in runOnModule():" << fileName << std::endl;
	std::cerr << "xyj---lineNo in runOnModule():" << lineNo <<std::endl;
	BasicBlock *BB = getBB(fileName, lineNo);
	if (BB) {
		endFunc = BB->getParent();
	}
	else {
		errs()<<"Error: get end function failed.\n";
		return false;
	}
	
	
	//to store temporary path
	std::vector<BasicBlock*> p;
	
	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i) {
		
		//Function &F = *i;
		Function *F = i;
		if (F)
		{
//			llvm::errs() << "CallFunction: " << F->getName() << "--------\n";
//			errs() << "\tCallees:\n";
		}
		else 
		{
			llvm::errs() << "***NULL Function***\n";
			continue;
		}
		cgNode = CG.getOrInsertFunction(F);
		F = cgNode->getFunction();
		for (CallGraphNode::iterator I = cgNode->begin(), E = cgNode->end();
				I != E; ++I){
			CallGraphNode::CallRecord *cr = &*I;

			if(cr->first){
				Instruction *TmpIns = dyn_cast<Instruction>(cr->first);
				if(!TmpIns)
//					errs() << "\t" << *TmpIns << "\n";
					continue;
			}
			Function *FI = cr->second->getFunction();
			
			//create the called graph;
			FunctionMapTy::iterator it = calledFunctionMap.find(FI);
			if(it==calledFunctionMap.end()) //not find
			{
				if(cr->first){
					calledFunctionMap[FI].push_back(std::make_pair(F, dyn_cast<Instruction>(cr->first)));
				}				
			}
			else{
				if(cr->first){
					it->second.push_back(std::make_pair(F, dyn_cast<Instruction>(cr->first)));
				}
			}
			
		}

	}

	llvm::errs() << "=================================\n";
	llvm::errs() << "get Function Path: " << endFunc->getName() 
		<< " to " << startFunc->getName() << " \n";
	
	printCalledFuncAndCFGPath(endFunc, startFunc, BB, &p);
	llvm::errs() << "on-end\n";
	llvm::errs() << "=================================\n";
	
	errs()<<"Find "<<paths_found->size()<<" paths in all.\n";
	paths::iterator ips = paths_found->begin();
	for(;ips != paths_found->end();ips++) {		
		for(std::vector<BasicBlock*>::iterator ps = ips->begin(), pe = ips->end(); ps != pe; ps++) {
			BasicBlock *tmpStr = *ps;
			errs()<<"\t"<<tmpStr->getParent()->getName()<<": "<<tmpStr->getName()<<" -> \n";
		}
		errs()<<"=================================\n";
	}
	
	return false;
}


