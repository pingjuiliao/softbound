//===-- Softbound.h - Example Transformations ------------------*- C++ -*-===//
//
// My softbound implementation 
// 
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_UTILS_SOFTBOUND_H
#define LLVM_TRANSFORMS_UTILS_SOFTBOUND_H
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#define SOFTBOUND_REGISTER "_softbound_register"
#define SOFTBOUND_UPDATE "_softbound_update"
#define SOFTBOUND_CHECK  "_softbound_check"

namespace llvm {

class SoftboundPass : public PassInfoMixin<SoftboundPass> {
public:
  typedef unsigned PointerID;
  PointerID AssignedID ;
  PreservedAnalyses run(Module&, ModuleAnalysisManager&);
  static bool isRequired() { return true;  } 
  SmallMapVector<Value*, PointerID, 0x100> PointerIDMap ;
private:

  // the main body of this passes
  bool initializeLinkage(Module *M) ;
  void registerPointers(Module &M) ;
  void checkPointers(Module &M) ;
  
  // should return AllocaInst or GlobalVariable
  
  // register
  void registerArray(AllocaInst*, ArrayType*)  ;
  void registerGlobalArray(GlobalVariable*, ArrayType*, Function*)  ;
  void registerAllocatedPointer(AllocaInst*, PointerType*) ;
  void registerGlobalPointer(GlobalVariable *, PointerType *, Function*) ;
  void registerArgPointer(Value*, Function*) ;
  void registerHeapAlloc(Instruction *I) ;
  void registerAndUpdatePHINode(PHINode *PHI) ;
  // update
  void updateOnArgs(CallInst*, Value*) ;
  void updateOnStore(StoreInst *StoreI) ; // unused (aggressive)
  void writeUpdateCodeAfter(Instruction* I, unsigned DstID) ;  
  // void writeReallocCodeAfter(Instruction *I) ;
  // check
  void checkDereference(Instruction &GEPInst) ;
  void checkSequentialWrite(Instruction &I) ;
  void writeCheckCodeAfter2(GetElementPtrInst *GEP, uint64_t offset) ;
  void writeCheckCodeAfter(GetElementPtrInst *GEP, Value* SizeVal) ;
  
  Value* getDeclaration(Value *V) ; 


}; // SoftboundPass end

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_SOFTBOUND_H
