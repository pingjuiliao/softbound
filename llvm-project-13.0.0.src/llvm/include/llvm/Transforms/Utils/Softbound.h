//===-- Softbound.h - Example Transformations ------------------*- C++ -*-===//
//
// My softbound implementation 
// 
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_UTILS_SOFTBOUND_H
#define LLVM_TRANSFORMS_UTILS_SOFTBOUND_H

#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

class SoftboundPass : public PassInfoMixin<SoftboundPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  // static bool isRequired() { return true;  } 
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_SOFTBOUND_H
