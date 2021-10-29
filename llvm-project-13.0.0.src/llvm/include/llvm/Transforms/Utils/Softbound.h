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

namespace llvm {

class SoftboundPass : public PassInfoMixin<SoftboundPass> {
public:
  typedef unsigned PointerID;
  PointerID assignedID ;
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  // static bool isRequired() { return true;  } 
private:

  SmallMapVector<Value*,PointerID , 0x100> PtrMap ; 

  // collect all the pointers and map them to PtrMap
  void harvestPointers(Function &F) ;

  // check functions such as strcmp, memcpy
  void checkSequentialCopy(Instruction &I) ;

}; // SoftboundPass end

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_SOFTBOUND_H
