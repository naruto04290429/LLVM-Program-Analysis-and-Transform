#define DEBUG_TYPE "hello"
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Attributes.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <algorithm>
#include <stack>
#include <set>

using namespace llvm;

namespace {
	struct Hello :  public FunctionPass {
		static char ID;                           
		Hello() : FunctionPass(ID) {}
	        //DEFINE_INTPASS_ANALYSIS_ADJUSTMENT(PointerAnalysisPass);
	        /**
	         * @brief Runs this pass on the given function.
	         * @param [in,out] func The function to analyze
	         * @return true if the function was modified; false otherwise
	        */
	        virtual bool runOnFunction(Function &F);
	};
}

char Hello::ID = 0;
GlobalVariable* GV = NULL;
static RegisterPass<Hello> X("hello", "Hello World Pass", false, false);

static void StoreReturnValue(Module* M, Function &F) {
    for(Function::iterator block = F.begin(); block != F.end(); block++) {
        for(BasicBlock::iterator inst = block->begin(); inst != block->end();inst++) {
            Instruction * Inst = inst;
            if(isa<ReturnInst>(Inst)) {
                ReturnInst* returnInst = dyn_cast<ReturnInst>(Inst);
                if(returnInst){
                    GV = new GlobalVariable(*M, IntegerType::get(M->getContext(), 32),
                    false,//constant? 
                    GlobalValue::ExternalLinkage, 0, "GV");
                    GV->setAlignment(4);
                    ConstantInt* const_int32_2 = ConstantInt::get(M->getContext(), APInt(32, StringRef("0"), 10));
                    GV->setInitializer(const_int32_2);
                    //Store the return value to the global variable
                    StoreInst* stinst = new StoreInst(returnInst->getReturnValue(), GV, false, returnInst);
                    stinst->setAlignment(4);
                }
            }
        }
    }
}

static void setCloneFunction(Module* M, Function &F) {
	for(Function::iterator block = F.begin(); block != F.end(); block++) {
        for(BasicBlock::iterator inst = block->begin(); inst != block->end();inst++) {
        	Instruction * Inst = inst;
        	if(isa<ReturnInst>(Inst)) {
                ReturnInst* returnInst = dyn_cast<ReturnInst>(Inst);
                Function* func_pop_direct_branch = M->getFunction("pop_direct_branch");
                CallInst* new_inst = CallInst::Create(func_pop_direct_branch, "", returnInst);
            }
        }
    }
}

bool Hello::runOnFunction(Function &F){
	bool has_modified = false;
    bool loadGlobal = false;

	errs() << "Current Function: " ;
	errs().write_escaped(F.getName())<< "\n";
	for(Function::iterator block = F.begin(); block != F.end(); block++) {
        for(BasicBlock::iterator inst = block->begin(); inst != block->end();inst++) {
    		Instruction* Inst = inst;
            errs() << *inst <<  "\n";
            
            StoreInst* temp_store;
            if(loadGlobal && isa<StoreInst>(temp_store)) {
            	Instruction* temp_inst = ++inst;inst--;
                temp_store = dyn_cast<StoreInst>(Inst);
                LoadInst* load_GV = new LoadInst(GV, "load_GV", temp_inst);
                load_GV->setAlignment(4);
                StoreInst* store_GV = 
                new StoreInst(load_GV, temp_store->getPointerOperand(), false, temp_inst);
                store_GV->setAlignment(4);
                loadGlobal = false;
            }

            if(isa<CallInst>(Inst)) {
            	ValueToValueMapTy VMap;//Can add without initialization
            	CallInst* callInst = dyn_cast<CallInst>(Inst);
				Function* callee = callInst->getCalledFunction();
            	
            	if(callee && callee->isDeclaration());
                //errs() << "Function is Declaration" << '\n';
                else if(callee && !callee->isDeclaration() &&
                    //!callee->doesNotReturn() &&  
                	callee->getName().str().find("p")==0 && 
                    callee->getName().str() !="pop_direct_branch") {
                    errs() << *callee;
                    
                    //CloneFunction to create clone_function
    				Function* clone_function = CloneFunction(callee, VMap, false);
                    clone_function->setLinkage(GlobalValue::InternalLinkage);
                    clone_function->setDoesNotReturn();
                    callee->getParent()->getFunctionList().push_back(clone_function);

                    //Determine whether the callee is void
                    bool is_void = callee->getReturnType()->isVoidTy();
                    if(!is_void && !callee->doesNotReturn()) {
                        //Store return Value to the global variable
                        StoreReturnValue(F.getParent(), *clone_function);
                        //Trigger the loadGlobal
                        loadGlobal = true;
                    }
                    if(!callee->doesNotReturn())
                        //Add pop_direct_branch
                        setCloneFunction(F.getParent(), *clone_function);
                    //Replace the original one with the clone one
                    callInst->setCalledFunction(clone_function);
                    //See the modified clone_function
                    //errs() << *clone_function << "\n";
    			}
		    }	
        }
    }
	return has_modified;
}
