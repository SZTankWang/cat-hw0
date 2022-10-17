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
#include "llvm/IR/CFG.h"
#include <unordered_map>
#include <iterator>
#include <set>
#include <list>
#include <typeinfo>
#include <utility>
#include <algorithm>
#include <string>
#include <vector>

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
//	use a map to record each cat_variable and instruction(pointer) that modifies/defines it
	std::unordered_map<std::string,std::list<Instruction*>> map_of_var;

//	a map that record the index:instruction mapping, this should not be necessary anymore
	std::unordered_map<std::string,std::string> map_of_inst;

//	the map that records the gen and kill of each instruction
	std::unordered_map<Instruction*,std::pair<std::list<Instruction*>,std::list<Instruction*>>> map_of_gen_kill;

	// a  map of instruction* to index
	std::unordered_map<Instruction*,int> map_inst_index;
	
// 	a map to map instruction * back to index
	std::unordered_map<int,Instruction*> map_index_inst;

// 	a map to store instruction and its gen
	std::unordered_map<Instruction*,std::vector<bool>> inst_gen;
//	a map to store instruction and its kill
	std::unordered_map<Instruction*,std::vector<bool>> inst_kill;
	int index = 0;
	for(bb = F.begin(); bb != F.end(); ++bb){
		BasicBlock::iterator i ;
		for(i = bb->begin(); i != bb->end(); ++i){

		//does the instruction modify a cat variable? 
			if (isa<CallInst>(i)){
			// convert the instruction to string
			std::string str_inst; 
			raw_string_ostream(str_inst)<< *i;



			// add the index:instruction mapping to map_of_inst
			//map_of_inst.insert({std::to_string(index),str_inst});
			
			//this is the name of the function (eg: cat_new)
			const char* name = cast<CallInst>(i)->getCalledFunction()->getName().data();

			//see if its an operation that modify cat var
			if(umap.find(name)!= umap.end()){

				if(i->getNumOperands() > 0){
					//set the instruction to index mapping
					map_inst_index.insert({&*i,index});
					map_index_inst.insert({index,&*i});
					index ++;

					//get the first  operand, which is the variable involved
					auto* operand = i->getOperand(0);
	
					//if operation is cat_new, take the inst string as the variable name
					std::string str_var; //string representation of the cat variable
					//is it a cat_New? if so, record its variable to map_of_var
					if(strcmp(name,"CAT_new")==0){
//						if it is a cat_new, the key we store in map_of_var should be the instruction

						raw_string_ostream(str_var)<<*i;

						if(map_of_var.find(str_var)==map_of_var.end()){
							std::list<Instruction*> new_set;

							new_set.push_back(&*i);//insert a pointer to the instruction

							map_of_var.insert({str_var,new_set});
						}
						else{
							map_of_var.at(str_var).push_back(&*i);
						}
						
				}
					else{
						raw_string_ostream(str_var)<< *operand;

					//for other cat modification instruction, add them according to the variable they are modifying
						if(map_of_var.find(str_var) == map_of_var.end()){
//							errs()<<"fail to find the variable in map_Of_var: "<<str_var<<"\n";
//							exit(0);
							
							std::list<Instruction*> new_set;

							new_set.push_back(&*i);//insert a pointer to the instruction
							map_of_var.insert({str_var,new_set});
					
						}
//						//insert the inst to the set it belongs to, again the index
						std::list<Instruction*>& the_set = map_of_var.at(str_var);
						the_set.push_back(&*i);
					}
				}
			}
		}
		
		// increment index

		}
	}
	
	
	//loop through the instructions again to determine the gen and kill set
//	errs()<<"INDEX IS NOW"<<index<<"\n";
	for(bb = F.begin(); bb != F.end(); ++bb){
		BasicBlock::iterator i ;
		for(i = bb->begin(); i != bb->end(); ++i){
//			errs()<<*i<<"\n";
		//does the instruction modify a cat variable? 

			std::vector<bool>kill_set(index);
			std::vector<bool>gen_set(index);

			//this is not necessaru anymore
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

			//	gen_set.insert({&*i,1});
				//get the index of the instruction
				gen_set[map_inst_index.find(&*i)->second] = true;


				//get map_of_var[var]
//				errs()<<"index:"<<index2<<"\n";
				std::list<Instruction*>& inst_list = map_of_var.at(str_var_2);
				for(Instruction* j:inst_list){
					if(&*i != j ){
//					kill_set.insert({j,1});
					kill_set[map_inst_index.find(j)->second] = true;
					}
					}

				}
			}
			//store gen and kill
			inst_gen.insert({&*i,gen_set});
			inst_kill.insert({&*i,kill_set});
//			errs()<<"CHECKING GEN SET SIZE:"<<(inst_gen.at(&*i))->size()<<"\n";
/*
			//print out the info
			errs()<<"INSTRUCTION:   "<<*i<<"\n";
			errs()<<"***************** GEN"<<"\n{\n";
			for (int i=0;i!=gen_set.size();i++){
				if(gen_set[i]==true){
					errs()<<*(map_index_inst.find(i)->second)<<"\n";
					}
				}
			errs()<<"}\n**************************************\n***************** KILL\n{\n";
			for (int i=0;i!=kill_set.size();i++){
				if(kill_set[i]==true){
					errs()<<*(map_index_inst.find(i)->second)<<"\n";

				}
				}
			errs()<<"}**************************************\n\n\n\n";
*/			
			//increment index
//			errs()<<"incrementing index2"<<"\n";

		}
	}



	//compute in and out set
	//need a map to store {bb:pair<in,out>}
	//a map to store {instruction: pair<in,out>}
	bool changed = true;
	std::unordered_map<Instruction*,std::vector<bool>*> inst_in;
	std::unordered_map<Instruction*,std::vector<bool>*> inst_out;
	//iterate through the basic block, for each basic block, the predecessor(s) of the first intruction is the terminator(s) of all the predecessors of its bb
	do{
	changed = false;
	for(auto &b : F){
	//does the basic block have ancestor? if it does, then compute the union of the out sets of its ancestors 
	//get predecessors, compute the predecessor's out union
//		errs()<<"a basic block"<<"\n";
		std::vector<bool>*  in= new std::vector<bool>(index,false); //the bool vector used for union operation
//		errs()<<"computing union of predecessor.."<<"\n";

		for(BasicBlock* ance:predecessors(&b)){
//		errs()<<"a predecessor"<<"\n";
		//get the terminator
			Instruction * termi = ance->getTerminator();
		// get old out set
			
			if(inst_out.find(termi)==inst_out.end()){
//			//that means we initialize the in and out set of the bb's terminator to null set
//				errs()<<"not found terminator's out"<<"\n";
				std::vector<bool>* new_out = new std::vector<bool>(index);
				inst_out.insert({termi,new_out});
			}
			else{
//				errs()<<"found terminator's out"<<"\n";
			}

			//compute union
			uni(in,inst_out.find(termi)->second,in,nullptr);


		}
		//record the result to the in set
		Instruction * first = &(b.front());
//		errs()<<"first instruction: "<<*first<<"\n";
		if(inst_in.find(first)!=inst_in.end()){
			
			inst_in.at(first) = in;
//			errs()<<"setting first's in set"<<"\n";
		}
		else{
//			errs()<<"inserting first's in set"<<"\n";
			inst_in.insert({first,in});
		}

		Instruction * prev=nullptr; //store the previous instruction pointer
		for(Instruction & I : b){
			//get its in and out, the in is the out of its predecessor
			std::vector<bool> * curr_in = new std::vector<bool>(index);
			if(prev == nullptr){
				//this is the front, its in set should be computed before entering this block
				*curr_in = *(inst_in.at(&I));
			}
			else{
				*curr_in = *(inst_out.at(prev));
			}
			//compute out set, we are going to compare curr_out with the new out
			std::vector<bool> * curr_out;
			std::vector<bool> * new_out = new std::vector<bool>(index);

			if(inst_out.find(&I)!=inst_out.end()){
//				errs()<<"found out set for curr inst"<<"\n";
				curr_out = inst_out.at(&I);
			}
			else{
//				errs()<<"out set not found!"<<"\n";
				curr_out = nullptr; //out hasnt been computed yet, assign nullptr
			}
				//the out of curr instruction hasnt been computed yet
			
			//out = gen(i) union (in(i) - kill(i))
			diff(curr_in,&(inst_kill.at(&I)),new_out);
//			errs()<<"GEN SET SIZE:"<<inst_gen.at(&I).size() <<"\n";
		//	myprint(inst_gen.at(&I));
			bool out_changed = uni(&(inst_gen.at(&I)),new_out,new_out,curr_out);
			//is new_out different from previous ?
			if (out_changed){
//				errs()<<"out changed!"<<"\n";
				changed = true;	
			}
			//update in and out set 
			if(inst_in.find(&I)==inst_in.end()){
				inst_in.insert({&I,curr_in});
			}
			else{
				inst_in.at(&I) = curr_in;

			}
			if(inst_out.find(&I)==inst_out.end()){
				inst_out.insert({&I,new_out});
			}
			else{
				inst_out.at(&I) = new_out;

			}

			//update prev
			prev = &I;
		}

		//after getting the in set of the basic block, we enter the bb to compute for each instruction
	}
	}
	while(changed == true);

	//print 
	for(inst_iterator I = inst_begin(&F),E=inst_end(&F);I!=E;++I){

			errs()<<"INSTRUCTION:   "<<*I<<"\n";
			errs()<<"***************** IN"<<"\n{\n";
			//does the instruction have in and out?
			if(inst_in.find(&*I)!=inst_in.end()){
				std::vector<bool>* in = inst_in.at(&*I);
				for (int i=0;i!=in->size();++i){
					if((*in)[i]==true){
						errs()<<*(map_index_inst.find(i)->second)<<"\n";
						}
					}

			}
			errs()<<"}\n**************************************\n***************** OUT\n{\n";
			if(inst_out.find(&*I)!=inst_out.end()){
				std::vector<bool>* out = inst_out.at(&*I);

				for (int i=0;i!=out->size();++i){
					if((*out)[i]==true){
						errs()<<*(map_index_inst.find(i)->second)<<"\n";

					}
				}


			}
			
			errs()<<"}**************************************\n\n\n\n";


		}
		
	//free memory here
	std::unordered_map<Instruction*,std::vector<bool>*>::iterator it = inst_in.begin();
	while(it!=inst_in.end()){

		delete it->second;
		it++;
		}
	it = inst_out.begin();
	while(it!=inst_out.end()){
		delete it->second;
		it++;
	}


  	return false;
}


//utility function to print out the content of a vector
	void myprint(std::vector<bool>* v){
		errs()<<"size of gen set"<<v->size()<<"\n";
		for(int i=0;i!=v->size();++i){
			errs()<<(*v)[i];
			}
		}


//utility function to compute difference between two vectors
	void diff(std::vector<bool>* left,std::vector<bool>* right,std::vector<bool>*result){
		for(int i=0;i!=left->size();++i){
			if((*left)[i] && !(*right)[i]){
				(*result)[i] = true;
			}
			if((*left)[i] && (*right)[i]){
				(*result)[i] = false;
			}
		}
	}

	//function to compute union between two vectors
	bool uni(std::vector<bool>* left,std::vector<bool>* right,std::vector<bool>* result,std::vector<bool>* old_out){
		bool changed = false;
		if(old_out == nullptr){
			changed = true;
		}
		for(int i=0;i!=left->size();++i){
			if((*left)[i] || (*right)[i]){
				if(old_out != nullptr && (*old_out)[i] != true){

				changed = true;
				}
				(*result)[i] = true;
			}
		}
		return changed;

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
