//===-- HelloWorld.cpp - Example Transformations --------------------------===//
//
//
//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/Softbound.h"

using namespace llvm;

void SoftboundPass::doInitialization(Function &F) {
    GlobalVariable LookupTable = 


}

PreservedAnalyses SoftboundPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
    static init = false ;
    if ( !init ) {
        doInitialization(Function &F) ;
        init = true ;
    }

    // get pointer first
    harvestPointers(F) ;
    
    // intrument checks
    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            // handle sequential map
            checkSequentialCopy(I) ;

        }
    }
    return PreservedAnalyses::all();
}

 

void SoftboundPass::harvestPointers(Function &F) {
    for ( auto &BB: F ) {
        for ( auto &I: BB ) {
            // only checks allocation intructions
            auto *AllocaI = dyn_cast<AllocaInst>(&I) ;
            if ( !AllocaI || !AllocaI.isArrayAllocation() ) continue ;

            // map char[]
            IRBuilder(I) IRB;
            SmallVector<Value*, 4> args() ;
            IRB.CreateCall("mapFatPointer") ;
        }
    }
}

void SoftboundPass::checkSequentialCopy(Instruction &I) {
    auto *CallI = dyn_cast<CallInst>(&I) ;
    if ( !CallI ) return ;


    // check  
    Function* Caller = CallI->getFunction() ;
    Function* F = CallI->getCalledFunction() ;
    errs() << Caller->getName() << " gonna check : " << F->getName() << "\n" ;

}

