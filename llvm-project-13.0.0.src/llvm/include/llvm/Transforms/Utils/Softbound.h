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
#define SOFTBOUND_CHECK  "_softbound_check"
#define SOFTBOUND_CHECK_OFFSET  "_softbound_check_offset"
#define SOFTBOUND_CHECK_STRING "_softbound_check_string"
namespace llvm {

class SoftboundPass : public PassInfoMixin<SoftboundPass> {
public:
  PreservedAnalyses run(Module&, ModuleAnalysisManager&);
  static bool isRequired() { return true;  } 
private:

  // the main body of this passes
  bool initializeLinkage(Module *M) ;
  void registerBaseBound(Module &M) ;
  void checkBaseBound(Module &M) ;
  
  // should return AllocaInst or GlobalVariable
  
  // register
  void registerStackBuffer(AllocaInst*)  ;
  void registerStackBuffer(GlobalVariable*, Function&);
  void registerHeapBuffer(CallInst*) ;
  // TODO: handle new operator
  
  // check
  void checkDereference(Instruction &GEPInst) ;
  void checkSizedSequentialOperation(Instruction &I) ;
  void checkStringBasedSequentialOperation(Instruction &I) ;
  // void checkFormatStringSequentialOperation(Instruction &I) ;
  void writeCheckCodeAfter(GetElementPtrInst *GEP, Value* SizeVal) ;
  


}; // SoftboundPass end

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_SOFTBOUND_H
