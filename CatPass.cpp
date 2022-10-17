#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include <unordered_map>
#include <iterator>
#include <set>
#include <list>
#include <typeinfo>
#include <utility>
#include <algorithm>
#include <string>

using namespace llvm;

namespace {
  struct CAT : public FunctionPass {
    static char ID; 

    CAT() : FunctionPass(ID) {}

    // This function is invoked once at the initialization phase of the compiler
    // The LLVM IR of functions isn't ready at this point
    bool doInitialization (Module &M) override {
  //    errs() << "Hello LLVM World at \"doInitialization\"\n" ;
      return false;
    }

    // This function is invoked once per function compiled
    // The LLVM IR of the input functions is ready and it can be analyzed and/or transformed
    bool runOnFunction (Function &F) override {
	//list of function that modify a cat var
	std::unordered_map<std::string,int> umap;

	umap["CAT_new"]=1;
	umap["CAT_set"]=1;
	umap["CAT_add"]=1;
	umap["CAT_sub"]=1;
	// print function name
	errs() << "Function "<<"\""<<F.getName()<<"\"\n";

//	for a function, get its entry block, and iterate starting from this block
	Function::iterator bb;
//	use a map to record each cat_variable and instruction that modifies/defines it
	std::unordered_map<std::string,std::list<int>> map_of_var;

//	a map that record the index:instruction mapping
	std::unordered_map<std::string,std::string> map_of_inst;

//	the map that records the gen and kill of each instruction
	std::unordered_map<std::string,std::pair<std::list<int>,std::list<int>>> map_of_gen_kill;

//	a counter to for holding the current index of the instruction
	signed int index=0;

	for(bb = F.begin(); bb != F.end(); ++bb){
		BasicBlock::iterator i ;
		for(i = bb->begin(); i != bb->end(); ++i){

		//does the instruction modify a cat variable? 
			if (isa<CallInst>(i)){
			// convert the instruction to string
			std::string str_inst; 
			raw_string_ostream(str_inst)<< *i;
			


			// add the index:instruction mapping to map_of_inst
			map_of_inst.insert({std::to_string(index),str_inst});
			//this is the name of the function (eg: cat_new)
			const char* name = cast<CallInst>(i)->getCalledFunction()->getName().data();

			//see if its an operation that modify cat var
			if(umap.find(name)!= umap.end()){

				if(i->getNumOperands() > 0){
					
					//get the first  operand, which is the variable involved
					auto* operand = i->getOperand(0);
	
					//if operation is cat_new, take the inst string as the variable name
					std::string str_var; //string representation of the cat variable
					//is it a cat_New? if so, record its variable to map_of_var
					if(strcmp(name,"CAT_new")==0){
//						if it is a cat_new, the key we store in map_of_var should be the instruction
						std::list<int> new_set;

						raw_string_ostream(str_var)<<*i;
						new_set.push_back(index);//insert the instruction's index
						map_of_var.insert({str_var,new_set});
				}
					else{
						raw_string_ostream(str_var)<< *operand;

					//for other cat modification instruction, add them according to the variable they are modifying
						if(map_of_var.find(str_var) == map_of_var.end()){
							errs()<<"fail to find the variable in map_Of_var"<<"\n";
							exit(0);
						}
//						//insert the inst to the set it belongs to, again the index
						std::list<int>& the_set = map_of_var.at(str_var);
						the_set.push_back(index);
					}
				}
			}
		}
		
		// increment index
		++index;

		}
	}
	
	//loop through the instructions again to determine the gen and kill set
	//reset index
	signed int index2 = 0; 
	for(bb = F.begin(); bb != F.end(); ++bb){
		BasicBlock::iterator i ;
		for(i = bb->begin(); i != bb->end(); ++i){
//			errs()<<*i<<"\n";
		//does the instruction modify a cat variable? 

			std::list<int> kill_set;
			std::list<int> gen_set;
			std::string str_inst_2; //string representation of the IR
			raw_string_ostream(str_inst_2)<<*i;

			if (isa<CallInst>(i)) {

			//this is the name of the function (eg: cat_new)
			const char* name = cast<CallInst>(i)->getCalledFunction()->getName().data();


			//see if its an operation that modify cat var
			

				if(i->getNumOperands() > 0 && umap.find(name)!= umap.end()){
					
					//get the first  operand, which is the variable involved
					auto* operand = i->getOperand(0);
	
					//if operation is cat_new, take the inst string as the variable name

					std::string str_var_2; //string representation of the cat variable

					if(strcmp(name,"CAT_new")==0){
//						if it is a cat_new, use the instruction as the key to map_of_var
						raw_string_ostream(str_var_2)<<*i;
						
					}
					else{
						raw_string_ostream(str_var_2)<< *operand;

					}
				//compute the gen/kill set,for kill set, do a loop on map_of_var[var] and select those that != index
				gen_set.push_back(index2);
				
				//get map_of_var[var]
//				errs()<<"index:"<<index2<<"\n";
				std::list<int>& inst_list = map_of_var.at(str_var_2);
				for(int i:inst_list){
					if(i != index2){
					kill_set.push_back(i);
					}
					}

				}
			}
			
			//print out the info
			errs()<<"INSTRUCTION:   "<<str_inst_2<<"\n";
			errs()<<"***************** GEN"<<"\n{\n";
			for (int x : gen_set){
				errs()<<map_of_inst.at(std::to_string(x))<<"\n";
				}
			errs()<<"}\n**************************************\n***************** KILL\n{\n";
			for (int x : kill_set){
				errs()<<map_of_inst.at(std::to_string(x))<<"\n";
				}
			errs()<<"}**************************************\n\n\n\n";
			
			//increment index
//			errs()<<"incrementing index2"<<"\n";
			index2 ++;

		}
	}


    	return false;
}

    // We don't modify the program, so we preserve all analyses.
    // The LLVM IR of functions isn't ready at this point
    void getAnalysisUsage(AnalysisUsage &AU) const override {
    //  errs() << "Hello LLVM World at \"getAnalysisUsage\"\n" ;
      AU.setPreservesAll();
    }
  };
}

// Next there is code to register your pass to "opt"
char CAT::ID = 0;
static RegisterPass<CAT> X("CAT", "Homework for the CAT class");

// Next there is code to register your pass to "clang"
static CAT * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT()); }}); // ** for -O0
