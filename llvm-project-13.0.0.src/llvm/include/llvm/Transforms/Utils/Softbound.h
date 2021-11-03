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
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/Support/raw_ostream.h"
#define SOFTBOUND_REGISTER "_softbound_register"
#define SOFTBOUND_UPDATE "_softbound_update"
#define SOFTBOUND_CHECK  "_softbound_check"

namespace llvm {

class SoftboundPass : public PassInfoMixin<SoftboundPass> {
public:
  typedef unsigned PointerID;
  PointerID AssignedID ;
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  // static bool isRequired() { return true;  } 
  SmallMapVector<Value*, PointerID, 0x100> PointerIDMap ;

private:

  // the main body of this passes
  bool initializeLinkage(Module *M) ;
  void harvestPointers(Function &F) ;
  void checkPointers(Function &F) ;
  
  // helpers
  
  // should return AllocaInst or GlobalVariable
  void registerPointer(AllocaInst *AllocaI, PointerType *PtrTy) ;
  void registerArray(AllocaInst *AllocaI, ArrayType *ArrTy)  ;
  void updatePointer(StoreInst *StoreI) ;
  void checkStore(Instruction &I) ;
  void checkSequentialWrite(Instruction &I) ;
  void writeCheckCode(Instruction *I, Value* FatPtr, Value* Ptr, uint64_t offset= 0);
  Value* getDeclaration(Value *V) ; 


}; // SoftboundPass end

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_SOFTBOUND_H
