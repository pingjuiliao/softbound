//===-- HelloWorld.cpp - Example Transformations --------------------------===//
//
//
//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/Softbound.h"

using namespace llvm;

PreservedAnalyses SoftboundPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
    errs() << F.getName() << "\n" ;
    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            errs() << I << "\n" ; 
        }
    }
    return PreservedAnalyses::all();
}
